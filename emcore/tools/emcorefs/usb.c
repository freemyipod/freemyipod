//
//
//    Copyright 2013 user890104
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

libusb_context *usb_ctx;
libusb_device_handle *usb_handle;
uint8_t usb_iface_num;

int32_t usb_init(void) {
    int32_t res;

#ifdef DEBUG
    fprintf(stderr, "Initialising USB library...\n");
#endif

    res = libusb_init(&usb_ctx);

    if (res != LIBUSB_SUCCESS) {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "USB library initialized!\n");
    libusb_set_debug(usb_ctx, 3);
#endif

    return LIBUSB_SUCCESS;
}

int32_t usb_find(uint16_t vendor_id, uint16_t product_id, uint8_t *reattach) {
    libusb_device **devs, *dev = NULL;
    ssize_t devs_cnt, i;
    uint8_t found = 0, j, k;
    int l;
    int32_t res;
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *cfg_desc;
    const struct libusb_interface *iface;
    const struct libusb_interface_descriptor *iface_desc;

#ifdef DEBUG
    fprintf(stderr, "Getting USB device list...\n");
#endif

    devs_cnt = libusb_get_device_list(usb_ctx, &devs);

    if (devs_cnt < 0) {
        return devs_cnt;
    }

#ifdef DEBUG
    fprintf(stderr, "Found %zd USB devices!\n", devs_cnt);
#endif
    for (i = 0; i < devs_cnt; ++i) {
        dev = devs[i];
#ifdef DEBUG
        fprintf(stderr, "Getting device descriptor of USB device %zd...\n", i);
#endif
        res = libusb_get_device_descriptor(dev, &dev_desc);

        if (res != LIBUSB_SUCCESS) {
#ifdef DEBUG
            fprintf(stderr, "Unable to get device descriptor of device %zd!\n", i);
#endif
            continue;
        }

#ifdef DEBUG
        fprintf(stderr, "[%04x:%04x] bus %d, device %d, USB ver. %04x\n", dev_desc.idVendor,
            dev_desc.idProduct, libusb_get_bus_number(dev),
            libusb_get_device_address(dev), dev_desc.bcdUSB);
#endif
        if (dev_desc.idVendor != vendor_id || dev_desc.idProduct != product_id) {
            continue;
        }
        
#ifdef DEBUG
        fprintf(stderr, "Found emCORE USB device!\n");
#endif
        if (!dev_desc.bNumConfigurations) {
#ifdef DEBUG
            fprintf(stderr, "No configs found...\n");
#endif
            continue;
        }

#ifdef DEBUG
        fprintf(stderr, "Found %u configs...\n", dev_desc.bNumConfigurations);
#endif

        for (j = 0; j < dev_desc.bNumConfigurations; ++j) {
#ifdef DEBUG
            fprintf(stderr, "Getting config descriptor %u of device...\n", j);
#endif

            res = libusb_get_config_descriptor(dev, j, &cfg_desc);

            if (res != LIBUSB_SUCCESS) {
                return res;
            }

            if (!cfg_desc->bNumInterfaces) {
#ifdef DEBUG
                fprintf(stderr, "No interfaces found...\n");
#endif
                continue;
            }

            for (k = 0; k < cfg_desc->bNumInterfaces; ++k) {
                iface = &cfg_desc->interface[k];

                if (!iface->num_altsetting) {
#ifdef DEBUG
                    fprintf(stderr, "No altsettings found...\n");
#endif
                    continue;
                }

                for (l = 0; l < iface->num_altsetting; ++l) {
                    iface_desc = &iface->altsetting[l];

                    if (
                        iface_desc->bInterfaceClass    != EMCORE_USB_INTERFACE_CLASS ||
                        iface_desc->bInterfaceSubClass != EMCORE_USB_INTERFACE_SUB_CLASS ||
                        iface_desc->bInterfaceProtocol != EMCORE_USB_INTERFACE_PROTOCOL
                    ) {
#ifdef DEBUG
                        fprintf(stderr, "Wrong interface class (%02X %02X %02X), trying next device...\n", iface_desc->bInterfaceClass, iface_desc->bInterfaceSubClass, iface_desc->bInterfaceProtocol);
#endif

                        continue;
                    }
                    
#ifdef DEBUG
                    fprintf(stderr, "emCORE Debugger interface at %u\n", iface_desc->bInterfaceNumber);
#endif

                    usb_iface_num = iface_desc->bInterfaceNumber;
                    found = 1;
                    goto outside_search_loop;
                }
            }
        }
    }
    
    outside_search_loop:

    if (found) {
        res = usb_open(dev, reattach);
    }
    else {
        fprintf(stderr, "emCORE Debugger interface not found!\n");

        res = EMCORE_ERROR_NO_DEVICE;
    }

#ifdef DEBUG
    fprintf(stderr, "Freeing device list...\n");
#endif
    libusb_free_device_list(devs, 1);

    return res;
}

int32_t usb_open(libusb_device *dev, uint8_t *reattach) {
    int32_t res;

#ifdef DEBUG
    fprintf(stderr, "Opening USB device...\n");
#endif
    res = libusb_open(dev, &usb_handle);

    if (res != LIBUSB_SUCCESS) {
        return res;
    }
#ifdef DEBUG
    fprintf(stderr, "USB device opened!\n");
    fprintf(stderr, "Setting USB configuration 1...\n");
#endif

    res = libusb_set_configuration(usb_handle, 1);

    if (res != LIBUSB_SUCCESS) {
        return res;
    }
    
#ifdef DEBUG
    fprintf(stderr, "USB configuration set!\n");
#endif
    res = libusb_kernel_driver_active(usb_handle, 0);

    if (res < 0) {
        return res;
    }
    
    if (res == 1) {
        *reattach = 1;

        res = libusb_detach_kernel_driver(usb_handle, 0);
    }

    if (res != LIBUSB_SUCCESS) {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "Claiming interface 0...\n");
#endif
    res = libusb_claim_interface(usb_handle, 0);

    if (res != LIBUSB_SUCCESS) {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "Interface claimed successfully!\n");
#endif

    return LIBUSB_SUCCESS;
}

int32_t usb_control_transfer(uint8_t rq_type, void *data, size_t length) {
    int32_t res;

    res = libusb_control_transfer(usb_handle, rq_type, 0, 0, usb_iface_num, (unsigned char *)data, length, 10000);

    if (res < 0) {
        return res;
    }

    if ((size_t)res != length) {
        return EMCORE_ERROR_INCOMPLETE;
    }

    return LIBUSB_SUCCESS;
}

int32_t usb_close(uint8_t reattach) {
    int32_t res;

#ifdef DEBUG
    fprintf(stderr, "Releasing USB interface...\n");
#endif
    res = libusb_release_interface(usb_handle, 0);

    if (res != LIBUSB_SUCCESS) {
        return res;
    }

#ifdef DEBUG
    fprintf(stderr, "Released interface successfully!\n");
#endif

    if (reattach) {
#ifdef DEBUG
        fprintf(stderr, "Reattaching kernel driver...\n");
#endif

        res = libusb_attach_kernel_driver(usb_handle, 0);

        if (res == LIBUSB_SUCCESS) {
#ifdef DEBUG
            fprintf(stderr, "Reattached successfully!\n");
#endif
        }
        else {
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

void usb_exit(void) {
#ifdef DEBUG
    fprintf(stderr, "Deinitializing USB library...\n");
#endif

    libusb_exit(usb_ctx);
}

int32_t usb_destroy(uint8_t reattach) {
    int32_t res = LIBUSB_SUCCESS;

    if (usb_handle) {
        res = usb_close(reattach);
    }

    if (usb_ctx) {
        usb_exit();
    }

    return res;
}
