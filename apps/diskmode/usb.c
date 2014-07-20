//
//
//    Copyright 2013 TheSeven
//    Copyright 2014 user890104
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


#include "emcoreapp.h"
#include "usb.h"
#include "ums.h"


int usb_maxlen;

static const struct usb_instance* usb_handle;
static union usb_endpoint_number usb_outep = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_OUT };
static union usb_endpoint_number usb_inep = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_IN };
static int maxpacket = 512;
static struct wakeup mainloop_wakeup;
static char* error = NULL;


static const struct usb_devicedescriptor usb_devicedescriptor =
{
    .bLength = sizeof(struct usb_devicedescriptor),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0xffff,
    .idProduct = 0xe001,
    .bcdDevice = 2,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};


static struct __attribute__((packed)) _usb_config1_descriptors
{
    struct usb_configurationdescriptor c1;
    struct usb_interfacedescriptor c1_i0_a0;
    struct usb_endpointdescriptor c1_i0_a0_e1out;
    struct usb_endpointdescriptor c1_i0_a0_e1in;
} usb_config1_descriptors =
{
    .c1 =
    {
        .bLength = sizeof(struct usb_configurationdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength = sizeof(struct _usb_config1_descriptors),
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = { .buspowered = 1, .selfpowered = 1 },
        .bMaxPower = 100 / 2,
    },
    .c1_i0_a0 =
    {
        .bLength = sizeof(struct usb_interfacedescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = 0x08,
        .bInterfaceSubClass = 0x06,
        .bInterfaceProtocol = 0x50,
        .iInterface = 0,
    },
    .c1_i0_a0_e1out =
    {
        .bLength = sizeof(struct usb_endpointdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_OUT },
        .bmAttributes = { .type = USB_ENDPOINT_ATTRIBUTE_TYPE_BULK },
        .wMaxPacketSize = 512,
        .bInterval = 1,
    },
    .c1_i0_a0_e1in =
    {
        .bLength = sizeof(struct usb_endpointdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_IN },
        .bmAttributes = { .type = USB_ENDPOINT_ATTRIBUTE_TYPE_BULK },
        .wMaxPacketSize = 512,
        .bInterval = 1,
    },
};


static const struct usb_stringdescriptor usb_string_language =
{
    .bLength = sizeof(usb_string_language) + sizeof(*usb_string_language.wString),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = { 0x0409 },
};


static const struct usb_stringdescriptor usb_string_vendor =
{
    .bLength = sizeof(usb_string_vendor) + sizeof(*usb_string_vendor.wString) * 14,
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = { 'f', 'r', 'e', 'e', 'm', 'y', 'i', 'p', 'o', 'd', '.', 'o', 'r', 'g' },
};


static const struct usb_stringdescriptor usb_string_product =
{
    .bLength = sizeof(usb_string_product) + sizeof(*usb_string_product.wString) * 16,
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = { 'e', 'm', 'C', 'O', 'R', 'E', ' ', 'D', 'i', 's', 'k', ' ', 'M', 'o', 'd', 'e' },
};


static const struct usb_stringdescriptor* usb_stringdescriptors[] =
{
    &usb_string_language,
    &usb_string_vendor,
    &usb_string_product,
};


int handle_ctrl_request(const struct usb_instance* data, int interface, union usb_ep0_buffer* request, const void** response)
{
    int size = -1;
    switch (request->setup.bmRequestType.type)
    {
    case USB_SETUP_BMREQUESTTYPE_TYPE_CLASS:
        switch (request->setup.bRequest.raw)
        {
        case 0xfe:  // GET_MAX_LUN
            data->buffer->raw[0] = 0;
            size = 1;
            break;
        case 0xff:  // STORAGE_RESET
            size = 0;
            break;
        default: break;
        }
        break;
        default: break;
    }
    return size;
}


void handle_set_altsetting(const struct usb_instance* data, int interface, int altsetting)
{
    usb_handle = data;
    usb_maxlen = maxpacket * MIN(usb_get_max_transfer_size(usb_handle, usb_outep),
                                 usb_get_max_transfer_size(usb_handle, usb_inep));
    usb_configure_ep(usb_handle, usb_outep, USB_ENDPOINT_TYPE_BULK, maxpacket);
    usb_configure_ep(usb_handle, usb_inep, USB_ENDPOINT_TYPE_BULK, maxpacket);
    ums_listen();
}


void handle_unset_altsetting(const struct usb_instance* data, int interface, int altsetting)
{
    usb_unconfigure_ep(usb_handle, usb_outep);
    usb_unconfigure_ep(usb_handle, usb_inep);
}


int handle_ep_ctrl_request(const struct usb_instance* data, int interface, int endpoint, union usb_ep0_buffer* request, const void** response)
{
    int size = -1;
    switch (request->setup.bmRequestType.type)
    {
    case USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD:
        switch (request->setup.bRequest.req)
        {
        case USB_SETUP_BREQUEST_CLEAR_FEATURE:
            if (request->setup.wLength || request->setup.wValue) break;
            ums_listen();
            break;
        default: break;
        }
        break;
        default: break;
    }
    return size;
}


void handle_xfer_complete(const struct usb_instance* data, int ifnum, int epnum, int bytesleft)
{
    ums_xfer_complete(epnum, bytesleft);
}


void handle_timeout(const struct usb_instance* data, int interface, int endpoint, int bytesleft)
{
    usb_set_stall(usb_handle, usb_outep, true);
    usb_set_stall(usb_handle, usb_inep, true);
}


static void handle_bus_reset(const struct usb_instance* data, int configuration, int interface, int highspeed)
{
    maxpacket = highspeed ? 512 : 64;
    usb_config1_descriptors.c1_i0_a0_e1out.wMaxPacketSize = maxpacket;
    usb_config1_descriptors.c1_i0_a0_e1in.wMaxPacketSize = maxpacket;
}


static struct usb_endpoint usb_c1_i0_a0_ep1out =
{
    .number = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_OUT },
    .ctrl_request = handle_ep_ctrl_request,
    .xfer_complete = handle_xfer_complete,
};


static struct usb_endpoint usb_c1_i0_a0_ep1in =
{
    .number = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_IN },
    .xfer_complete = handle_xfer_complete,
    .timeout = handle_timeout,
};


static const struct usb_altsetting usb_c1_i0_a0 =
{
    .set_altsetting = handle_set_altsetting,
    .unset_altsetting = handle_unset_altsetting,
    .endpoint_count = 2,
    .endpoints =
    {
        &usb_c1_i0_a0_ep1out,
        &usb_c1_i0_a0_ep1in,
    },
};


static const struct usb_interface usb_c1_i0 =
{
    .bus_reset = handle_bus_reset,
    .ctrl_request = handle_ctrl_request,
    .altsetting_count = 1,
    .altsettings =
    {
        &usb_c1_i0_a0,
    },
};


static const struct usb_configuration usb_c1 =
{
    .descriptor = &usb_config1_descriptors.c1,
    .set_configuration = NULL,
    .unset_configuration = NULL,
    .interface_count = 1,
    .interfaces =
    {
        &usb_c1_i0,
    },
};


static const struct usb_configuration* usb_configurations[] =
{
    &usb_c1,
};


void usb_prepare()
{
    int i;
    uint32_t eps = usbmanager_get_available_endpoints();
    int ep_out = 0;
    int ep_in = 0;
    for (i = 1; i < 16; i++)
        if (eps & (1 << i))
        {
            ep_out = i;
            break;
        }
    for (i = 1; i < 16; i++)
        if (eps & (1 << (16 + i)))
        {
            ep_in = i;
            break;
        }
    if (!ep_out || !ep_in) panicf(PANIC_KILLTHREAD, "Not enough USB endpoints available!\n");
    usb_config1_descriptors.c1_i0_a0_e1out.bEndpointAddress.number = ep_out;
    usb_config1_descriptors.c1_i0_a0_e1in.bEndpointAddress.number = ep_in;
    usb_c1_i0_a0_ep1out.number.number = ep_out;
    usb_c1_i0_a0_ep1in.number.number = ep_in;
    usb_outep.number = ep_out;
    usb_inep.number = ep_in;
}


void usb_connect()
{
    int rc = usbmanager_install_custom(&usb_devicedescriptor, ARRAYLEN(usb_configurations), usb_configurations,
                                       ARRAYLEN(usb_stringdescriptors), usb_stringdescriptors, true);
    if (IS_ERR(rc))
    {
        cprintf(3, "Failed to register USB handler: %08X\n", rc);
        return;
    }
    
    wakeup_init(&mainloop_wakeup);
    
    while (!ums_ejected && usbmanager_get_connected())
        if (wakeup_wait(&mainloop_wakeup, 200000) == THREAD_OK)
            ums_handle_async();
    
    usbmanager_uninstall_custom();
    
    if (error) panic(PANIC_KILLTHREAD, error);
}


void enqueue_async()
{
    wakeup_signal(&mainloop_wakeup);
}


void usb_transmit(void* buffer, uint32_t len)
{
    usb_start_tx(usb_handle, usb_inep, buffer, len);
}


void usb_receive(void* buffer, uint32_t len)
{
    usb_start_rx(usb_handle, usb_outep, buffer, len);
}


void usb_stall_in()
{
    usb_set_stall(usb_handle, usb_inep, true);
}


void usb_stall_out()
{
    usb_set_stall(usb_handle, usb_outep, true);
}


void fail(char* errormsg)
{
    error = errormsg;
    ums_ejected = true;
    wakeup_signal(&mainloop_wakeup);
}
