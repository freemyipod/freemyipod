#include "global.h"
#include "app/umsboot/usbglue.h"
#include "app/umsboot/ums.h"
#include "sys/util.h"
#include UMSBOOT_USB_DRIVER_HEADER

static const struct usb_devicedescriptor usb_devicedescriptor __attribute__((section(".dmarodata.usb_devicedescriptor"),aligned(CACHEALIGN_SIZE))) =
{
    .bLength = sizeof(struct usb_devicedescriptor),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = UMSBOOT_USB_VENDORID,
    .idProduct = UMSBOOT_USB_PRODUCTID,
    .bcdDevice = UMSBOOT_USB_DEVICEREVISION,
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
} usb_config1_descriptors __attribute__((section(".dmadata.usb_config1_descriptors"),aligned(CACHEALIGN_SIZE))) =
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
        .bMaxPower = UMSBOOT_USB_MAXCURRENT / 2,
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
        .bEndpointAddress = { .number = UMSBOOT_ENDPOINT_OUT, .direction = USB_ENDPOINT_DIRECTION_OUT },
        .bmAttributes = { .type = USB_ENDPOINT_ATTRIBUTE_TYPE_BULK },
        .wMaxPacketSize = 512,
        .bInterval = 1,
    },
    .c1_i0_a0_e1in =
    {
        .bLength = sizeof(struct usb_endpointdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = { .number = UMSBOOT_ENDPOINT_IN, .direction = USB_ENDPOINT_DIRECTION_IN },
        .bmAttributes = { .type = USB_ENDPOINT_ATTRIBUTE_TYPE_BULK },
        .wMaxPacketSize = 512,
        .bInterval = 1,
    },
};

static const struct usb_stringdescriptor usb_string_language __attribute__((section(".dmarodata.usb_string_language"),aligned(CACHEALIGN_SIZE))) =
{
    .bLength = sizeof(usb_string_language) + sizeof(*usb_string_language.wString),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = { 0x0409 },
};

static const struct usb_stringdescriptor usb_string_vendor __attribute__((section(".dmarodata.usb_string_vendor"),aligned(CACHEALIGN_SIZE))) =
{
    .bLength = sizeof(usb_string_vendor) + sizeof(*usb_string_vendor.wString) * UMSBOOT_USB_VENDORSTRING_LEN,
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = UMSBOOT_USB_VENDORSTRING,
};

static const struct usb_stringdescriptor usb_string_product __attribute__((section(".dmarodata.usb_string_product"),aligned(CACHEALIGN_SIZE))) =
{
    .bLength = sizeof(usb_string_product) + sizeof(*usb_string_product.wString) * UMSBOOT_USB_PRODUCTSTRING_LEN,
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = UMSBOOT_USB_PRODUCTSTRING,
};

static const struct usb_stringdescriptor* usb_stringdescriptors[] =
{
    &usb_string_language,
    &usb_string_vendor,
    &usb_string_product,
};

static const struct usb_endpoint usb_c1_i0_a0_ep1out =
{
    .number = { .number = UMSBOOT_ENDPOINT_OUT, .direction = USB_ENDPOINT_DIRECTION_OUT },
    .ctrl_request = ums_ep_ctrl_request,
    .xfer_complete = ums_xfer_complete,
    .setup_received = NULL,
};

static const struct usb_endpoint usb_c1_i0_a0_ep1in =
{
    .number = { .number = UMSBOOT_ENDPOINT_IN, .direction = USB_ENDPOINT_DIRECTION_IN },
    .xfer_complete = ums_xfer_complete,
    .timeout = ums_timeout,
};

static const struct usb_altsetting usb_c1_i0_a0 =
{
    .set_altsetting = ums_set_altsetting,
    .unset_altsetting = ums_unset_altsetting,
    .endpoint_count = 2,
    .endpoints =
    {
        &usb_c1_i0_a0_ep1out,
        &usb_c1_i0_a0_ep1in,
    },
};

static void usbglue_bus_reset(const struct usb_instance* data, int highspeed)
{
    usb_config1_descriptors.c1_i0_a0_e1out.wMaxPacketSize = highspeed ? 512 : 64;
    usb_config1_descriptors.c1_i0_a0_e1in.wMaxPacketSize = highspeed ? 512 : 64;
}

static struct usb_interface usb_c1_i0 =
{
    .bus_reset = ums_bus_reset,
    .ctrl_request = ums_ctrl_request,
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


static UMSBOOT_USB_DRIVER_CONFIG_TYPE usb_driver_config = UMSBOOT_USB_DRIVER_CONFIG;

static UMSBOOT_USB_DRIVER_STATE_TYPE usb_driver_state = UMSBOOT_USB_DRIVER_STATE;

static struct usb_state usb_state;

static union usb_ep0_buffer usb_buffer __attribute__((section(".dmabss.usb_buffer"),aligned(CACHEALIGN_SIZE)));

const struct usb_instance usb_data =
{
    .driver = &UMSBOOT_USB_DRIVER,
    .driver_config = &usb_driver_config,
    .driver_state = &usb_driver_state,
    .state = &usb_state,
    .buffer = &usb_buffer,
    .bus_reset = usbglue_bus_reset,
    .ep0_setup_hook = NULL,
    .ep0_data_hook = NULL,
    .configuration_count = 1,
    .stringdescriptor_count = 3,
    .devicedescriptor = &usb_devicedescriptor,
    .stringdescriptors = usb_stringdescriptors,
    .configurations =
    {
        &usb_c1,
    },
};
