#include "global.h"
#include "usbglue.h"
#include "usb.h"
#include "usbdebug.h"
#include "util.h"
#include "thread.h"
#include "malloc.h"
#include USB_DRIVER_HEADER

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
    .idProduct = 0xe000,
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
        .bMaxPower = USB_MAXCURRENT / 2,
    },
    .c1_i0_a0 =
    {
        .bLength = sizeof(struct usb_interfacedescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = 0xff,
        .bInterfaceSubClass = 0x00,
        .bInterfaceProtocol = 0x00,
        .iInterface = 0,
    },
    .c1_i0_a0_e1out =
    {
        .bLength = sizeof(struct usb_endpointdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = { .number = USBDEBUG_ENDPOINT_OUT, .direction = USB_ENDPOINT_DIRECTION_OUT },
        .bmAttributes = { .type = USB_ENDPOINT_ATTRIBUTE_TYPE_BULK },
        .wMaxPacketSize = 512,
        .bInterval = 1,
    },
    .c1_i0_a0_e1in =
    {
        .bLength = sizeof(struct usb_endpointdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = { .number = USBDEBUG_ENDPOINT_IN, .direction = USB_ENDPOINT_DIRECTION_IN },
        .bmAttributes = { .type = USB_ENDPOINT_ATTRIBUTE_TYPE_BULK },
        .wMaxPacketSize = 512,
        .bInterval = 1,
    },
};

static const struct usb_interfacedescriptor usb_simpledebug_intf_desc =
{
    .bLength = sizeof(struct usb_interfacedescriptor),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 0,
    .bInterfaceClass = 0xff,
    .bInterfaceSubClass = 0x00,
    .bInterfaceProtocol = 0x00,
    .iInterface = 0,
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
    .bLength = sizeof(usb_string_product) + sizeof(*usb_string_product.wString) * 15,
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = { 'e', 'm', 'C', 'O', 'R', 'E', ' ', 'd', 'e', 'b', 'u', 'g', 'g', 'e', 'r' },
};

static const struct usb_stringdescriptor* usb_stringdescriptors[] =
{
    &usb_string_language,
    &usb_string_vendor,
    &usb_string_product,
};

static const struct usb_endpoint usb_c1_i0_a0_ep1out =
{
    .number = { .number = USBDEBUG_ENDPOINT_OUT, .direction = USB_ENDPOINT_DIRECTION_OUT },
    .ctrl_request = usbdebug_bulk_ctrl_request,
    .xfer_complete = usbdebug_bulk_xfer_complete,
    .setup_received = NULL,
};

static const struct usb_endpoint usb_c1_i0_a0_ep1in =
{
    .number = { .number = USBDEBUG_ENDPOINT_IN, .direction = USB_ENDPOINT_DIRECTION_IN },
    .ctrl_request = usbdebug_bulk_ctrl_request,
    .xfer_complete = usbdebug_bulk_xfer_complete,
    .timeout = NULL,
};

static const struct usb_altsetting usb_c1_i0_a0 =
{
    .set_altsetting = usbdebug_bulk_enable,
    .unset_altsetting = usbdebug_bulk_disable,
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
    .bus_reset = usbdebug_bus_reset,
    .ctrl_request = usbdebug_handle_setup,
    .altsetting_count = 1,
    .altsettings =
    {
        &usb_c1_i0_a0,
    },
};

static const struct usb_altsetting usb_simpledebug_intf_a0 =
{
    .set_altsetting = usbdebug_enable,
    .unset_altsetting = usbdebug_disable,
    .endpoint_count = 0,
    .endpoints =
    {
    },
};

static struct usb_interface usb_simpledebug_intf =
{
    .bus_reset = NULL,
    .ctrl_request = usbdebug_handle_setup,
    .altsetting_count = 1,
    .altsettings =
    {
        &usb_simpledebug_intf_a0,
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


static USB_DRIVER_CONFIG_TYPE usb_driver_config = USB_DRIVER_CONFIG;

static USB_DRIVER_STATE_TYPE usb_driver_state = USB_DRIVER_STATE;

static struct usb_state usb_state =
{
    .interface_altsetting = { 0 },
};

static union usb_ep0_buffer usb_buffer CACHEALIGN_ATTR;

struct usb_instance usb_default_data =
{
    .driver = &USB_DRIVER,
    .driver_config = &usb_driver_config,
    .driver_state = &usb_driver_state,
    .state = &usb_state,
    .buffer = &usb_buffer,
    .bus_reset = usbglue_bus_reset,
    .ep0_setup_hook = NULL,
    .configuration_count = 1,
    .stringdescriptor_count = 3,
    .devicedescriptor = &usb_devicedescriptor,
    .stringdescriptors = usb_stringdescriptors,
    .configurations =
    {
        &usb_c1,
    },
};

struct usb_instance* usb_data = &usb_default_data;

static struct scheduler_thread usbmanager_thread_handle;
static uint32_t usbmanager_stack[0x40] STACK_ATTR;
static bool usb_connected;
static struct mutex usbmanager_mutex;

void usbmanager_thread(void* arg0, void* arg1, void* arg2, void* arg3)
{
    while (true)
    {
        sleep(200000);
        mutex_lock(&usbmanager_mutex, TIMEOUT_BLOCK);
        bool newstate = vbus_state();
        if (usb_connected != newstate)
        {
            if (newstate) usb_init(usb_data);
            else usb_exit(usb_data);
            usb_connected = newstate;
        }
        mutex_unlock(&usbmanager_mutex);
    }
}

void usbmanager_init()
{
    mutex_init(&usbmanager_mutex);
    usb_connected = false;
    usbdebug_init();
    
    thread_create(&usbmanager_thread_handle, "synopsysotg", usbmanager_thread,
                  usbmanager_stack, sizeof(usbmanager_stack), OS_THREAD, 63, true,
                  NULL, NULL, NULL, NULL);
}

void usbmanager_exit()
{
    if (usb_connected) usb_exit(usb_data);
}

int usbmanager_install_custom(const struct usb_devicedescriptor* devicedescriptor,
                              uint8_t config_count, const struct usb_configuration** configurations,
                              uint8_t string_count, const struct usb_stringdescriptor** stringdescriptors,
                              bool enable_debug)
{
    int i, j, k, l;
    int size = sizeof(struct usb_instance);
    int descsize = sizeof(struct usb_devicedescriptor);
    int max_interfaces = 0;
    for (i = 0; i < config_count; i++)
    {
        const struct usb_configuration* configuration = configurations[i];
        size += 4 + sizeof(struct usb_configuration);
        descsize += configuration->descriptor->wTotalLength;
        if (configuration->interface_count > max_interfaces)
            max_interfaces = configuration->interface_count;
        for (j = 0; j < configuration->interface_count; j++)
        {
            const struct usb_interface* interface = configuration->interfaces[j];
            size += 4 + sizeof(struct usb_interface);
            for (k = 0; k < interface->altsetting_count; k++)
            {
                const struct usb_altsetting* altsetting = interface->altsettings[k];
                size += 4 + sizeof(struct usb_altsetting)
                      + (4 + sizeof(struct usb_endpoint)) * altsetting->endpoint_count;
            }
        }
    }
    for (i = 0; i < string_count; i++) descsize += stringdescriptors[i]->bLength;
    if (enable_debug)
    {
        size += 4;
        descsize += sizeof(struct usb_interfacedescriptor);
        if (configurations[0]->interface_count == max_interfaces) max_interfaces++;
    }
    size += 4 * string_count + sizeof(struct usb_state) + max_interfaces;
    void* buf = malloc(size + descsize);
    if (!buf) RET_ERR(1);
    reownalloc(buf, KERNEL_OWNER(KERNEL_OWNER_CUSTOM_USB));
    void* descbuf = buf + size;
    struct usb_instance* instance = (struct usb_instance*)buf;
    buf += sizeof(struct usb_instance) + 4 * config_count;
    memcpy(instance, &usb_default_data, sizeof(struct usb_instance));
    struct usb_devicedescriptor* devdescriptor = (struct usb_devicedescriptor*)descbuf;
    descbuf += sizeof(struct usb_devicedescriptor);
    memcpy(devdescriptor, devicedescriptor, sizeof(struct usb_devicedescriptor));
    instance->devicedescriptor = devdescriptor;
    for (i = 0; i < config_count; i++)
    {
        const struct usb_configuration* configuration = configurations[i];
        struct usb_configuration* config = (struct usb_configuration*)buf;
        buf += sizeof(struct usb_configuration) + 4 * configuration->interface_count;
        memcpy(config, configuration, sizeof(struct usb_configuration));
        instance->configurations[i] = config;
        struct usb_configurationdescriptor* cfgdescriptor = (struct usb_configurationdescriptor*)descbuf;
        descbuf += configuration->descriptor->wTotalLength;
        memcpy(cfgdescriptor, configuration->descriptor, configuration->descriptor->wTotalLength);
        config->descriptor = cfgdescriptor;
        if (!i && enable_debug) buf += 4;
        for (j = 0; j < configuration->interface_count; j++)
        {
            const struct usb_interface* interface = configuration->interfaces[j];
            struct usb_interface* intf = (struct usb_interface*)buf;
            buf += sizeof(struct usb_interface) + 4 * interface->altsetting_count;
            memcpy(intf, interface, sizeof(struct usb_interface));
            config->interfaces[j] = intf;
            for (k = 0; k < interface->altsetting_count; k++)
            {
                const struct usb_altsetting* altsetting = interface->altsettings[k];
                struct usb_altsetting* as = (struct usb_altsetting*)buf;
                buf += sizeof(struct usb_altsetting) + 4 * altsetting->endpoint_count;
                memcpy(as, altsetting, sizeof(struct usb_altsetting));
                intf->altsettings[k] = as;
                for (l = 0; l < altsetting->endpoint_count; l++)
                {
                    const struct usb_endpoint* endpoint = altsetting->endpoints[l];
                    struct usb_endpoint* ep = (struct usb_endpoint*)buf;
                    buf += sizeof(struct usb_endpoint);
                    memcpy(ep, endpoint, sizeof(struct usb_endpoint));
                    as->endpoints[l] = ep;
                }
            }
        }
        if (!i && enable_debug)
        {
            struct usb_interfacedescriptor* intfdescriptor = (struct usb_interfacedescriptor*)descbuf;
            descbuf += sizeof(struct usb_interfacedescriptor);
            memcpy(intfdescriptor, &usb_simpledebug_intf_desc, sizeof(struct usb_interfacedescriptor));
            intfdescriptor->bInterfaceNumber = j;
            config->interfaces[j] = &usb_simpledebug_intf;
            config->interface_count++;
            cfgdescriptor->wTotalLength += sizeof(struct usb_interfacedescriptor);
            cfgdescriptor->bNumInterfaces++;
        }
    }
    const struct usb_stringdescriptor** stringdescs = (const struct usb_stringdescriptor**)buf;
    buf += 4 * string_count;
    instance->stringdescriptors = stringdescs;
    for (i = 0; i < string_count; i++)
    {
        const struct usb_stringdescriptor* stringdescriptor = stringdescriptors[i];
        struct usb_stringdescriptor* stringdesc = (struct usb_stringdescriptor*)descbuf;
        descbuf += stringdescriptor->bLength;
        memcpy(stringdesc, stringdescriptor, stringdescriptor->bLength);
        stringdescs[i] = stringdesc;
    }
    struct usb_state* state = (struct usb_state*)buf;
    buf += sizeof(struct usb_state) + max_interfaces;
    instance->state = state;
    mutex_lock(&usbmanager_mutex, TIMEOUT_BLOCK);
    if (usb_connected) usb_exit(usb_data);
    if (usb_data != &usb_default_data) free(usb_data);
    usb_data = instance;
    if (usb_connected) usb_init(usb_data);
    mutex_unlock(&usbmanager_mutex);
    return 0;
}

void usbmanager_uninstall_custom()
{
    mutex_lock(&usbmanager_mutex, TIMEOUT_BLOCK);
    if (usb_data != &usb_default_data)
    {
        if (usb_connected) usb_exit(usb_data);
        free(usb_data);
        usb_data = &usb_default_data;
        if (usb_connected) usb_init(usb_data);
    }
    mutex_unlock(&usbmanager_mutex);
}

uint32_t usbmanager_get_available_endpoints()
{
    return USB_ENDPOINTS;
}

bool usbmanager_get_connected()
{
    return usb_connected;
}

