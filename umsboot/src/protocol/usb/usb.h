#ifndef __PROTOCOL_USB_USB_H__
#define __PROTOCOL_USB_USB_H__

#include "global.h"

struct usb_instance;

union __attribute__((packed,aligned(4))) usb_ep0_buffer
{
    struct __attribute__((packed,aligned(4)))
    {
        struct __attribute__((packed))
        {
            enum
            {
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_DEVICE = 0,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_INTERFACE = 1,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_ENDPOINT = 2,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_OTHER = 3,
            } recipient : 5;
            enum
            {
                USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD = 0,
                USB_SETUP_BMREQUESTTYPE_TYPE_CLASS = 1,
                USB_SETUP_BMREQUESTTYPE_TYPE_VENDOR = 2,
            } type : 2;
            enum
            {
                USB_SETUP_BMREQUESTTYPE_DIRECTION_OUT = 0,
                USB_SETUP_BMREQUESTTYPE_DIRECTION_IN = 1,
            } direction : 1;
        } bmRequestType;
        union
        {
            enum __attribute__((packed))
            {
                USB_SETUP_BREQUEST_GET_STATUS = 0,
                USB_SETUP_BREQUEST_CLEAR_FEATURE = 1,
                USB_SETUP_BREQUEST_SET_FEATURE = 3,
                USB_SETUP_BREQUEST_SET_ADDRESS = 5,
                USB_SETUP_BREQUEST_GET_DESCRIPTOR = 6,
                USB_SETUP_BREQUEST_SET_DESCRIPTOR = 7,
                USB_SETUP_BREQUEST_GET_CONFIGURATION = 8,
                USB_SETUP_BREQUEST_SET_CONFIGURATION = 9,
                USB_SETUP_BREQUEST_GET_INTERFACE = 10,
                USB_SETUP_BREQUEST_SET_INTERFACE = 11,
                USB_SETUP_BREQUEST_SYNCH_FRAME = 12,
            } req;
            uint8_t raw;
        } bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    } setup;
    uint8_t raw[64];
};

enum __attribute__((packed)) usb_descriptor_type
{
    USB_DESCRIPTOR_TYPE_DEVICE = 1,
    USB_DESCRIPTOR_TYPE_CONFIGURATION = 2,
    USB_DESCRIPTOR_TYPE_STRING = 3,
    USB_DESCRIPTOR_TYPE_INTERFACE = 4,
    USB_DESCRIPTOR_TYPE_ENDPOINT = 5,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER = 6,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIG = 7,
    USB_DESCRIPTOR_TYPE_INTERFACE_POWER = 8,
    USB_DESCRIPTOR_TYPE_OTG = 9,
    USB_DESCRIPTOR_TYPE_DEBUG = 10,
    USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION = 11,
};

enum usb_endpoint_direction
{
    USB_ENDPOINT_DIRECTION_OUT = 0,
    USB_ENDPOINT_DIRECTION_IN = 1,
};

union __attribute__((packed)) usb_endpoint_number
{
    struct __attribute__((packed))
    {
        int number : 4;
        int reserved: 3;
        enum usb_endpoint_direction direction : 1;
    };
    uint8_t byte;
};

enum usb_endpoint_type
{
    USB_ENDPOINT_TYPE_CONTROL = 0,
    USB_ENDPOINT_TYPE_ISOCHRONOUS = 1,
    USB_ENDPOINT_TYPE_BULK = 2,
    USB_ENDPOINT_TYPE_INTERRUPT = 3,
};

struct __attribute__((packed)) usb_devicedescriptor
{
    uint8_t bLength;
    enum usb_descriptor_type bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
};

struct __attribute__((packed)) usb_configurationdescriptor
{
    uint8_t bLength;
    enum usb_descriptor_type bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    struct __attribute__((packed))
    {
        unsigned int reserved : 5;
        unsigned int remotewakeup : 1;
        unsigned int selfpowered : 1;
        unsigned int buspowered : 1;
    } bmAttributes;
    uint8_t bMaxPower;
};

struct __attribute__((packed)) usb_interfacedescriptor
{
    uint8_t bLength;
    enum usb_descriptor_type bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
};

struct __attribute__((packed)) usb_endpointdescriptor
{
    uint8_t bLength;
    enum usb_descriptor_type bDescriptorType;
    union usb_endpoint_number bEndpointAddress;
    struct __attribute__((packed))
    {
        enum
        {
            USB_ENDPOINT_ATTRIBUTE_TYPE_CONTROL = 0,
            USB_ENDPOINT_ATTRIBUTE_TYPE_ISOCHRONOUS = 1,
            USB_ENDPOINT_ATTRIBUTE_TYPE_BULK = 2,
            USB_ENDPOINT_ATTRIBUTE_TYPE_INTERRUPT = 3,
        } type : 2;
        enum
        {
            USB_ENDPOINT_ATTRIBUTE_SYNCTYPE_NOSYNC = 0,
            USB_ENDPOINT_ATTRIBUTE_SYNCTYPE_ASYNC = 1,
            USB_ENDPOINT_ATTRIBUTE_SYNCTYPE_ADAPTIVE = 2,
            USB_ENDPOINT_ATTRIBUTE_SYNCTYPE_SYNC = 3,
        } synctype : 2;
        enum
        {
            USB_ENDPOINT_ATTRIBUTE_USAGE_DATA = 0,
            USB_ENDPOINT_ATTRIBUTE_USAGE_FEEDBACK = 1,
            USB_ENDPOINT_ATTRIBUTE_USAGE_EXPLICIT = 2,
        } usage : 2;
        unsigned int reserved : 2;
    } bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
};

struct __attribute__((packed)) usb_stringdescriptor
{
    uint8_t bLength;
    enum usb_descriptor_type bDescriptorType;
    uint16_t wString[];
};

struct __attribute__((packed,aligned(4))) usb_endpoint
{
    union usb_endpoint_number number;
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    int (*ctrl_request)(const struct usb_instance* data, int interface, int endpoint, union usb_ep0_buffer* request, const void** response);
    void (*xfer_complete)(const struct usb_instance* data, int interface, int endpoint, int bytesleft);
    union __attribute__((packed))
    {
        void (*setup_received)(const struct usb_instance* data, int interface, int endpoint);
        void (*timeout)(const struct usb_instance* data, int interface, int endpoint, int bytesleft);
    };
};

struct __attribute__((packed,aligned(4))) usb_altsetting
{
    void (*set_altsetting)(const struct usb_instance* data, int interface, int altsetting);
    void (*unset_altsetting)(const struct usb_instance* data, int interface, int altsetting);
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t endpoint_count;
    const struct usb_endpoint* endpoints[];
};

struct __attribute__((packed,aligned(4))) usb_interface
{
    void (*bus_reset)(const struct usb_instance* data, int configuration, int interface, int highspeed);
    int (*ctrl_request)(const struct usb_instance* data, int interface, union usb_ep0_buffer* request, const void** response);
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t altsetting_count;
    const struct usb_altsetting* altsettings[];
};

struct __attribute__((packed,aligned(4))) usb_configuration
{
    const struct usb_configurationdescriptor* descriptor;
    void (*set_configuration)(const struct usb_instance* data, int configuration);
    void (*unset_configuration)(const struct usb_instance* data, int configuration);
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t interface_count;
    const struct usb_interface* interfaces[];
};

struct __attribute__((packed,aligned(4))) usb_state
{
    bool (*ep0_rx_callback)(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft);
    bool (*ep0_tx_callback)(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft);
    const void* ep0_tx_ptr;
    uint16_t ep0_tx_len;
    uint8_t current_address;
    uint8_t current_configuration;
    uint8_t interface_altsetting[];
};

struct __attribute__((packed,aligned(4))) usb_driver
{
    void (*init)(const struct usb_instance* data);
    void (*exit)(const struct usb_instance* data);
    void (*ep0_start_rx)(const struct usb_instance* data, bool non_setup, int len);
    void (*ep0_start_tx)(const struct usb_instance* data, const void* buf, int len);
    void (*start_rx)(const struct usb_instance* data, union usb_endpoint_number ep, void* buf, int size);
    void (*start_tx)(const struct usb_instance* data, union usb_endpoint_number ep, const void* buf, int size);
    int (*get_stall)(const struct usb_instance* data, union usb_endpoint_number ep);
    void (*set_stall)(const struct usb_instance* data, union usb_endpoint_number ep, bool stall);
    void (*set_address)(const struct usb_instance* data, uint8_t address);
    void (*configure_ep)(const struct usb_instance* data, union usb_endpoint_number ep, enum usb_endpoint_type type, int maxpacket);
    void (*unconfigure_ep)(const struct usb_instance* data, union usb_endpoint_number ep);
    int (*get_max_transfer_size)(const struct usb_instance* data, union usb_endpoint_number ep);
};

struct __attribute__((packed,aligned(4))) usb_instance
{
    const struct usb_driver* driver;
    const void* driver_config;
    void* driver_state;
    struct usb_state* state;
    union usb_ep0_buffer* buffer;
    void (*bus_reset)(const struct usb_instance* data, int highspeed);
    int (*ctrl_request)(const struct usb_instance* data, union usb_ep0_buffer* request, const void** response);
    int (*ep0_setup_hook)(const struct usb_instance* data, union usb_ep0_buffer* buf);
    uint8_t configuration_count;
    uint8_t stringdescriptor_count;
    uint8_t reserved1;
    uint8_t reserved2;
    const struct usb_devicedescriptor* devicedescriptor;
    const struct usb_stringdescriptor** stringdescriptors;
    const struct usb_configuration* configurations[];
};

extern void usb_init(const struct usb_instance* data);
extern void usb_exit(const struct usb_instance* data);
extern void usb_handle_bus_reset(const struct usb_instance* data, int highspeed);
extern void usb_handle_timeout(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft);
extern void usb_handle_xfer_complete(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft);
extern void usb_handle_setup_received(const struct usb_instance* data, union usb_endpoint_number epnum);
extern void usb_ep0_start_rx(const struct usb_instance* data, bool non_setup, int len, bool (*callback)(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft));
extern void usb_ep0_start_tx(const struct usb_instance* data, const void* buf, int len, bool (*callback)(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft));
extern void usb_start_rx(const struct usb_instance* data, union usb_endpoint_number ep, void* buf, int size);
extern void usb_start_tx(const struct usb_instance* data, union usb_endpoint_number ep, const void* buf, int size);
extern void usb_set_stall(const struct usb_instance* data, union usb_endpoint_number ep, bool stall);
extern void usb_configure_ep(const struct usb_instance* data, union usb_endpoint_number ep, enum usb_endpoint_type type, int maxpacket);
extern void usb_unconfigure_ep(const struct usb_instance* data, union usb_endpoint_number ep);
extern int usb_get_max_transfer_size(const struct usb_instance* data, union usb_endpoint_number ep);
extern bool usb_ep0_tx_callback(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft);
extern bool usb_ep0_short_tx_callback(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft);
extern bool usb_ep0_ack_callback(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft);


#endif
