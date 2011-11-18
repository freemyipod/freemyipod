//
//
//    Copyright 2011 user890104
//
//
//    This file is part of emCORE.
//
//    emCORE is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emCORE is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include "global.h"

#include "emcore.h"
#include "util.h"
#include "usb.h"


libusb_context* usb_ctx = NULL;
libusb_device_handle* usb_handle = NULL;

extern struct emcore_usb_endpoints_addr emcore_usb_eps_addr;
extern struct emcore_usb_endpoints_max_packet_size emcore_usb_eps_mps;

int usb_init(void)
{
    int res;

#ifdef DEBUG
    fprintf(stderr, "Initialising USB library...\n");
#endif

    res = libusb_init(&usb_ctx);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "USB library initialized!\n");
    libusb_set_debug(usb_ctx, 3);

#endif

    return LIBUSB_SUCCESS;
}

int usb_find(uint16_t vendor_id, uint16_t product_id, uint8_t* reattach)
{
    libusb_device **devs, *dev;
    ssize_t devs_cnt;
    int res, i;
    struct libusb_device_descriptor dev_desc;
    uint8_t found = 0;
    struct libusb_config_descriptor* cfg_desc;
    const struct libusb_interface* iface;
    const struct libusb_interface_descriptor* iface_desc;
    const struct libusb_endpoint_descriptor* ep_desc;

#ifdef DEBUG
    fprintf(stderr, "Getting USB device list...\n");
#endif

    devs_cnt = libusb_get_device_list(usb_ctx, &devs);

    if (devs_cnt < 0)
    {
        return devs_cnt;
    }

#ifdef DEBUG
    fprintf(stderr, "Found %d USB devices!\n", devs_cnt);
#endif
    for (i = 0; i < devs_cnt; ++i)
    {
        dev = devs[i];
#ifdef DEBUG
        fprintf(stderr, "Getting device descriptor of USB device %d...\n", i);
#endif
        res = libusb_get_device_descriptor(dev, &dev_desc);

        if (LIBUSB_SUCCESS != res)
        {
#ifdef DEBUG
            fprintf(stderr, "Unable to get device descriptor of device %d!\n", i);
#endif
            continue;
        }

#ifdef DEBUG
        fprintf(stderr, "[%04x:%04x] bus %d, device %d, USB ver. %04x\n", dev_desc.idVendor,
            dev_desc.idProduct, libusb_get_bus_number(dev),
            libusb_get_device_address(dev), dev_desc.bcdUSB);
#endif
        if (vendor_id == dev_desc.idVendor && product_id == dev_desc.idProduct)
        {
#ifdef DEBUG
            fprintf(stderr, "Found emCORE USB device!\n");
#endif
            if (1 != dev_desc.bNumConfigurations)
            {
#ifdef DEBUG
                fprintf(stderr, "Number of configs is different than 1, not the right device...\n");
#endif
                continue;
            }

#ifdef DEBUG
            fprintf(stderr, "Getting config descriptor 0 of device...\n");
#endif

            res = libusb_get_config_descriptor(dev, 0, &cfg_desc);

            if (LIBUSB_SUCCESS != res)
            {
                return res;
            }

            if (1 != cfg_desc->bNumInterfaces)
            {
#ifdef DEBUG
                fprintf(stderr, "Wrong USB device, it should have exactly 1 interface\n");
#endif

                continue;
            }

            iface = &cfg_desc->interface[0];

            if (1 != iface->num_altsetting)
            {
#ifdef DEBUG
                fprintf(stderr, "Wrong USB device, it should have exactly 1 altsetting\n");
#endif

                continue;
            }

            iface_desc = &iface->altsetting[0];

            if (4 != iface_desc->bNumEndpoints)
            {
#ifdef DEBUG
                fprintf(stderr, "Wrong USB device, it should have exactly 4 endpoints\n");
#endif

                continue;
            }

#ifdef DEBUG
           fprintf(stderr, "Endpoints:");
#endif

            for (i = 0; i < 4; ++i)
            {
                ep_desc = &iface_desc->endpoint[i];

#ifdef DEBUG
                fprintf(stderr, " %d at 0x%02x", i, ep_desc->bEndpointAddress);
#endif

                switch (i) {
                    case 0:
                        emcore_usb_eps_addr.cout = ep_desc->bEndpointAddress;
                    break;
                    case 1:
                        emcore_usb_eps_addr.cin = ep_desc->bEndpointAddress;
                    break;
                    case 2:
                        emcore_usb_eps_addr.dout = ep_desc->bEndpointAddress;
                    break;
                    case 3:
                        emcore_usb_eps_addr.din = ep_desc->bEndpointAddress;
                    break;
                }
            }

#ifdef DEBUG
            fprintf(stderr, "\n");
#endif

            found = 1;
        }
    }

    if (found)
    {
        res = usb_open(dev, reattach);
    }
    else
    {
        fprintf(stderr, "USB device with VID=%4x and PID=%4x not found!\n",
            vendor_id, product_id);

        res = EMCORE_ERROR_NO_DEVICE;
    }

#ifdef DEBUG
    fprintf(stderr, "Freeing device list...\n");
#endif
    libusb_free_device_list(devs, 1);

    return res;
}

int usb_open(libusb_device* dev, uint8_t* reattach)
{
    int res;

#ifdef DEBUG
    fprintf(stderr, "Opening USB device...\n");
#endif
    res = libusb_open(dev, &usb_handle);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
#ifdef DEBUG
    fprintf(stderr, "USB device opened!\n");
    fprintf(stderr, "Setting USB configuration 1...\n");
#endif

    res = libusb_set_configuration(usb_handle, 1);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
#ifdef DEBUG
    fprintf(stderr, "USB configuration set!\n");
#endif
    res = libusb_kernel_driver_active(usb_handle, 0);

    if (1 == res)
    {
        *reattach = 1;

        res = libusb_detach_kernel_driver(usb_handle, 0);
    }

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "Claiming interface 0...\n");
#endif
    res = libusb_claim_interface(usb_handle, 0);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "Interface claimed successfully!\n");
    fprintf(stderr, "Getting endpoints max size...\n");
#endif

    res = emcore_get_packet_info(&emcore_usb_eps_mps);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "Got endpoint max size!\n");
    fprintf(stderr, "COUT max pckt: %d, CIN max pckt: %d, DOUT max pckt: %d, DIN max pckt: %d\n",
        emcore_usb_eps_mps.cout, emcore_usb_eps_mps.cin, emcore_usb_eps_mps.dout, emcore_usb_eps_mps.din
    );
#endif

    return LIBUSB_SUCCESS;
}

int usb_bulk_transfer(unsigned char endpoint, void* data, int length)
{
    int transferred;
    int res;

    res = libusb_bulk_transfer(usb_handle, endpoint, (unsigned char*)data, length, &transferred, 30000);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }

    if (transferred != length)
    {
        return EMCORE_ERROR_INCOMPLETE;
    }

    return LIBUSB_SUCCESS;
}

int usb_close(uint8_t reattach) {
    int res;

#ifdef DEBUG
    fprintf(stderr, "Releasing USB interface...\n");
#endif
    res = libusb_release_interface(usb_handle, 0);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "Released interface successfully!\n");
#endif

    if (reattach)
    {
#ifdef DEBUG
        fprintf(stderr, "Reattaching kernel driver...\n");
#endif

        res = libusb_attach_kernel_driver(usb_handle, 0);

        if (LIBUSB_SUCCESS == res)
        {
#ifdef DEBUG
            fprintf(stderr, "Reattached successfully!\n");
#endif
        }
        else
        {
            print_error(res);

            res = LIBUSB_SUCCESS;
        }
    }
#ifdef DEBUG
    fprintf(stderr, "Closing USB device handle...\n");
#endif
    libusb_close(usb_handle);

    return res;
}

void usb_exit(void)
{
#ifdef DEBUG
    fprintf(stderr, "Deinitializing USB library...\n");
#endif

    libusb_exit(usb_ctx);
}

int usb_destroy(uint8_t reattach)
{
    int res = LIBUSB_SUCCESS;

    if (usb_handle)
    {
        res = usb_close(reattach);
    }

    if (usb_ctx)
    {
        usb_exit();
    }

    return res;
}
