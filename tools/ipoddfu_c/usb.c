//
//
//    Copyright 2011 user890104
//
//
//    This file is part of ipoddfu.
//
//    ipoddfu is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    ipoddfu is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with ipoddfu.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include <stdio.h>

#include <libusb-1.0/libusb.h>

#include "misc.h"
#include "usb.h"

libusb_context *usb_ctx = NULL;
libusb_device_handle *usb_handle = NULL;

int usb_init(void)
{
    int res;
    
    printf("Initialising USB library... ");
    
    res = libusb_init(&usb_ctx);
    
    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
    
    printf("OK\n");
    
#ifdef DEBUG
    libusb_set_debug(usb_ctx, 3);
#endif

    return LIBUSB_SUCCESS;
}

int usb_find(unsigned char *reattach)
{
    libusb_device **devs, *dev;
    ssize_t devs_cnt;
    int res, i;
    struct libusb_device_descriptor dev_desc;
    unsigned char found = 0;
    struct libusb_config_descriptor *cfg_desc;
    const struct libusb_interface *iface;
    const struct libusb_interface_descriptor *iface_desc;
    
    printf("Getting USB device list... ");
    
    devs_cnt = libusb_get_device_list(usb_ctx, &devs);
        
    if (devs_cnt < 0)
    {
        return devs_cnt;
    }
    
    printf("Found %Zd USB devices!\n", devs_cnt);
            
    for (i = 0; i < devs_cnt; ++i)
    {
        dev = devs[i];
        
        printf("Getting device descriptor of USB device %d...\n", i);

        res = libusb_get_device_descriptor(dev, &dev_desc);

        if (LIBUSB_SUCCESS != res)
        {
            fprintf(stderr, "Unable to get device descriptor of device %d!\n", i);
            
            continue;
        }
        
        printf("[%04x:%04x] bus %d, device %d, USB ver. %04x\n", dev_desc.idVendor,
            dev_desc.idProduct, libusb_get_bus_number(dev),
            libusb_get_device_address(dev), dev_desc.bcdUSB);

        if (0x05ac == dev_desc.idVendor && (
            // DFU
            0x1220 == dev_desc.idProduct || // iPod Nano 2G
            0x1223 == dev_desc.idProduct || // iPod Nano 3G and iPod Classic 1G/2G/3G
            0x1224 == dev_desc.idProduct || // iPod Nano 3G
            0x1225 == dev_desc.idProduct || // iPod Nano 4G
            0x1231 == dev_desc.idProduct || // iPod Nano 5G
            0x1232 == dev_desc.idProduct || // iPod Nano 6G
            0x1233 == dev_desc.idProduct || // iPod Shuffle 4G
            // WTF
            0x1240 == dev_desc.idProduct || // iPod Nano 2G
            0x1241 == dev_desc.idProduct || // iPod Classic 1G
            0x1242 == dev_desc.idProduct || // iPod Nano 3G
            0x1243 == dev_desc.idProduct || // iPod Nano 4G
            0x1245 == dev_desc.idProduct || // iPod Classic 2G
            0x1246 == dev_desc.idProduct || // iPod Nano 5G
            0x1247 == dev_desc.idProduct || // iPod Classic 3G
            0x1248 == dev_desc.idProduct    // iPod Nano 6G
        ))
        {
            printf("Found DFU USB device!\n");
            
            if (1 != dev_desc.bNumConfigurations)
            {
                fprintf(stderr, "Number of configs is different than 1, not the right device...\n");
                
                continue;
            }
            
            printf("Getting config descriptor 0 of device...\n");

            res = libusb_get_config_descriptor(dev, 0, &cfg_desc);

            if (LIBUSB_SUCCESS != res)
            {
                return res;
            }
            
            if (1 != cfg_desc->bNumInterfaces)
            {
                fprintf(stderr, "Wrong USB device, it should have exactly 1 interface\n");
                
                continue;
            }
            
            iface = &cfg_desc->interface[0];

            if (1 != iface->num_altsetting)
            {
                fprintf(stderr, "Wrong USB device, it should have exactly 1 altsetting\n");
                
                continue;
            }
            
            iface_desc = &iface->altsetting[0];
            
            if (0 != iface_desc->bNumEndpoints)
            {
                fprintf(stderr, "Wrong USB device, it should have no endpoints\n");
                
                continue;
            }
            
            found = 1;
            break;
        }
    }

    if (found)
    {
        res = usb_open(dev, reattach);
    }
    else
    {
        fprintf(stderr, "DFU USB device not found!\n");
        
        res = 1; // not found
    }
    
    printf("Freeing device list...\n");

    libusb_free_device_list(devs, 1);

    return res;
}

int usb_open(libusb_device *dev, unsigned char *reattach)
{
    int res;
    
    printf("Opening USB device... ");

    res = libusb_open(dev, &usb_handle);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
    
    printf("OK\n");

    printf("Setting USB configuration 1... ");

    res = libusb_set_configuration(usb_handle, 1);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
    
    printf("OK\n");

    res = libusb_kernel_driver_active(usb_handle, 0);

    if (1 == res)
    {
        printf("Kernel driver active, detaching... ");
        
        *reattach = 1;

        res = libusb_detach_kernel_driver(usb_handle, 0);
        
        if (LIBUSB_SUCCESS == res)
        {
            printf("OK\n");
        }
    }

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
    
    printf("Claiming interface 0... ");

    res = libusb_claim_interface(usb_handle, 0);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
    
    printf("OK\n");
    
    return LIBUSB_SUCCESS;
}

int usb_bulk_transfer(const unsigned char endpoint, unsigned char *data, const unsigned int length)
{
    int transferred;
    int res;
    
    res = libusb_bulk_transfer(usb_handle, endpoint, data, length, &transferred, 30000);
    
    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
    
    if ((unsigned int) transferred != length)
    {
        return 2; // incomplete
    }
    
    return LIBUSB_SUCCESS;
}

int usb_control_transfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength)
{
    int res;
    
    res = libusb_control_transfer(usb_handle, bmRequestType, bRequest, wValue, wIndex, data, wLength, 30000);
    
    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
    
    return LIBUSB_SUCCESS;
}

int usb_close(const unsigned char reattach)
{
    int res;
    
    printf("Releasing USB interface... ");
    
    res = libusb_release_interface(usb_handle, 0);
    
    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }
    
    printf("OK\n");
    
    if (reattach)
    {
        printf("Reattaching kernel driver... ");
        
        res = libusb_attach_kernel_driver(usb_handle, 0);
        
        if (LIBUSB_SUCCESS == res)
        {
            printf("OK\n");
        }
        else
        {
            printf("\n");
            print_error(res);
            
            res = LIBUSB_SUCCESS;
        }
    }
    
    printf("Closing USB device handle...\n");
    
    libusb_close(usb_handle);
    
    return res;
}

void usb_exit(void)
{
    printf("Deinitializing USB library...\n");

    libusb_exit(usb_ctx);
}
