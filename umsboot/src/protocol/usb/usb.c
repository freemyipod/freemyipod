#include "global.h"
#include "protocol/usb/usb.h"

void usb_start_rx(const struct usb_instance* data, union usb_endpoint_number ep, void* buf, int size)
{
    data->driver->start_rx(data, ep, buf, size);
}

void usb_start_tx(const struct usb_instance* data, union usb_endpoint_number ep, const void* buf, int size)
{
    data->driver->start_tx(data, ep, buf, size);
}

void usb_set_stall(const struct usb_instance* data, union usb_endpoint_number ep, int stall)
{
    data->driver->set_stall(data, ep, stall);
}

void usb_ep0_start_rx(const struct usb_instance* data, int non_setup)
{
    data->driver->ep0_start_rx(data, non_setup);
}

void usb_ep0_start_tx(const struct usb_instance* data, const void* buf, int len)
{
    // Expect zero-length ACK if we are about to actually send data, otherwise expect SETUP.
    usb_ep0_start_rx(data, !!len);

    data->driver->ep0_start_tx(data, buf, len);
}

void usb_ep0_expect_setup(const struct usb_instance* data)
{
    // The next packet needs to be a SETUP, so lock out everything on the IN pipe.
    union usb_endpoint_number ep = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_IN };
    usb_set_stall(data, ep, 1);

    // Set up the OUT pipe for the SETUP packet, STALLing everything else.
    usb_ep0_start_rx(data, 0);
}

void usb_unconfigure_ep(const struct usb_instance* data, union usb_endpoint_number ep)
{
    data->driver->unconfigure_ep(data, ep);
}

void usb_configure_ep(const struct usb_instance* data, union usb_endpoint_number ep, enum usb_endpoint_type type, int maxpacket)
{
    // Reset the endpoint, just in case someone left it in a dirty state.
    usb_unconfigure_ep(data, ep);

    // Write the new configuration for the endpoint.
    // This resets the data toggle to DATA0, as required by the USB specification.
    data->driver->configure_ep(data, ep, type, maxpacket);
}

int usb_get_max_transfer_size(const struct usb_instance* data, union usb_endpoint_number ep)
{
    return data->driver->get_max_transfer_size(data, ep);
}

static void usb_unconfigure(const struct usb_instance* data)
{
    // Notify a configuration and its active interface altsettings that it was just kicked out.
    int configid = data->state->current_configuration;
    if (!configid) return;
    const struct usb_configuration* configuration = data->configurations[configid - 1];
    int i;
    for (i = 0; i < configuration->interface_count; i++)
    {
        struct usb_interface* interface = configuration->interfaces[i];
        const struct usb_altsetting* altsetting = interface->altsettings[interface->current_altsetting];
        if (altsetting->unset_altsetting)
            altsetting->unset_altsetting(data, i, interface->current_altsetting);
    }
    if (configuration->unset_configuration)
        configuration->unset_configuration(data, configid);
    data->state->current_configuration = 0;
}

static const struct usb_endpoint* usb_find_endpoint(const struct usb_instance* data,
                                                    union usb_endpoint_number ep, int* ifidx, int* epidx)
{
    // Figure out who's currently owning a (hardware) endpoint.
    // Returns a pointer to the usb_endpoint struct, and sets *epidx to its index within its altsetting.
    // *ifidx will be set to the interface number within the current configuration that the altsetting belongs to.
    // If nobody currently owns the specified endpoint, return NULL (*epidx and *ifidx will contain garbage).
    int configid = data->state->current_configuration;
    if (!configid) return NULL;
    const struct usb_configuration* configuration = data->configurations[configid - 1];
    for (*ifidx = 0; *ifidx < configuration->interface_count; (*ifidx)++)
    {
        const struct usb_interface* interface = configuration->interfaces[*ifidx];
        const struct usb_altsetting* altsetting = interface->altsettings[interface->current_altsetting];
        for (*epidx = 0; *epidx < altsetting->endpoint_count; (*epidx)++)
        {
            const struct usb_endpoint* endpoint = altsetting->endpoints[*epidx];
            if (endpoint->number.byte == ep.byte) return endpoint;
        }
    }
    return NULL;
}

static void usb_handle_ep0_setup(const struct usb_instance* data, union usb_ep0_buffer* buffer)
{
    // size < -2: do nothing at all (dangerous, you need to take care of priming EP0 yourself!)
    // size == -2: send STALL
    // size == -1: try to run default handler, or send STALL if none exists
    // size == 0: send ACK
    // size > 0: send <size> bytes at <addr>, then expect ACK
    const void* addr = NULL;
    int size = -1;
    switch (buffer->setup.bmRequestType.recipient)
    {
    case USB_SETUP_BMREQUESTTYPE_RECIPIENT_DEVICE:
        if (data->ctrl_request) size = data->ctrl_request(data, buffer, &addr);
        if (size != -1) break;
        switch (buffer->setup.bmRequestType.type)
        {
        case USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD:
            switch (buffer->setup.bRequest.req)
            {
            case USB_SETUP_BREQUEST_GET_STATUS:
                if (buffer->setup.wLength != 2 || buffer->setup.wIndex
                 || !data->state->current_address || buffer->setup.wValue) break;
                data->buffer->raw[0] = 0;
                data->buffer->raw[1] = 1;
                addr = data->buffer;
                size = 2;
                break;
            case USB_SETUP_BREQUEST_SET_ADDRESS:
                if (buffer->setup.wLength || buffer->setup.wIndex || buffer->setup.wValue > 127) break;
                data->state->current_address = buffer->setup.wValue;
                // We can already set the address here, the driver has to take care to send the ACK with the old address.
                data->driver->set_address(data, data->state->current_address);
                size = 0;
                break;
            case USB_SETUP_BREQUEST_GET_DESCRIPTOR:
                if (!buffer->setup.wLength) break;
                switch (buffer->setup.wValue >> 8)
                {
                case USB_DESCRIPTOR_TYPE_DEVICE:
                    if ((buffer->setup.wValue & 0xff) || buffer->setup.wIndex) break;
                    addr = data->devicedescriptor;
                    size = data->devicedescriptor->bLength;
                    break;
                case USB_DESCRIPTOR_TYPE_CONFIGURATION:
                    if (buffer->setup.wIndex
                     || (buffer->setup.wValue & 0xff) >= data->configuration_count) break;
                    addr = data->configurations[buffer->setup.wValue & 0xff]->descriptor;
                    size = data->configurations[buffer->setup.wValue & 0xff]->descriptor->wTotalLength;
                    break;
                case USB_DESCRIPTOR_TYPE_STRING:
                    if ((buffer->setup.wValue & 0xff) > data->stringdescriptor_count) break;
                    addr = data->stringdescriptors[buffer->setup.wValue & 0xff];
                    size = data->stringdescriptors[buffer->setup.wValue & 0xff]->bLength;
                    break;
                }
                if (size > buffer->setup.wLength) size = buffer->setup.wLength;
                break;
            case USB_SETUP_BREQUEST_GET_CONFIGURATION:
                if (buffer->setup.wLength != 1 || buffer->setup.wIndex
                 || !data->state->current_address || buffer->setup.wValue) break;
                data->buffer->raw[0] = data->state->current_configuration;
                addr = data->buffer;
                size = 1;
                break;
            case USB_SETUP_BREQUEST_SET_CONFIGURATION:
                if (buffer->setup.wLength || buffer->setup.wIndex || !data->state->current_address
                 || buffer->setup.wValue > data->configuration_count) break;
                usb_unconfigure(data);
                data->state->current_configuration = buffer->setup.wValue;
                if (data->state->current_configuration)
                {
                    // Notify the configuration and its interface default altsettings that it should set up stuff
                    int configid = data->state->current_configuration;
                    const struct usb_configuration* configuration = data->configurations[configid - 1];
                    if (configuration->set_configuration)
                        configuration->set_configuration(data, configid);
                    int i;
                    for (i = 0; i < configuration->interface_count; i++)
                    {
                        struct usb_interface* interface = configuration->interfaces[i];
                        interface->current_altsetting = 0;
                        const struct usb_altsetting* altsetting = interface->altsettings[0];
                        if (altsetting->set_altsetting) altsetting->set_altsetting(data, i, 0);
                    }
                }
                size = 0;
                break;
            default: break;
            }
            break;
        }
        break;
    case USB_SETUP_BMREQUESTTYPE_RECIPIENT_INTERFACE:
    {
        if (!data->state->current_configuration) break;
        int configid = data->state->current_configuration;
        const struct usb_configuration* configuration = data->configurations[configid - 1];
        if (buffer->setup.wIndex >= configuration->interface_count) break;
        int intfid = buffer->setup.wIndex;
        struct usb_interface* interface = configuration->interfaces[intfid];
        if (interface->ctrl_request) size = interface->ctrl_request(data, intfid, buffer, &addr);
        if (size != -1) break;
        switch (buffer->setup.bmRequestType.type)
        {
        case USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD:
            switch (buffer->setup.bRequest.req)
            {
            case USB_SETUP_BREQUEST_GET_STATUS:
                if (buffer->setup.wLength != 2 || buffer->setup.wValue) break;
                data->buffer->raw[0] = 0;
                data->buffer->raw[1] = 0;
                addr = data->buffer;
                size = 2;
                break;
            case USB_SETUP_BREQUEST_GET_INTERFACE:
                if (buffer->setup.wLength != 1 || buffer->setup.wValue) break;
                data->buffer->raw[0] = interface->current_altsetting;
                addr = data->buffer;
                size = 1;
                break;
            case USB_SETUP_BREQUEST_SET_INTERFACE:
            {
                if (buffer->setup.wLength
                 || buffer->setup.wValue > interface->altsetting_count) break;
                const struct usb_altsetting* altsetting = interface->altsettings[interface->current_altsetting];
                if (altsetting->unset_altsetting)
                    altsetting->unset_altsetting(data, intfid, interface->current_altsetting);
                interface->current_altsetting = buffer->setup.wValue;
                altsetting = interface->altsettings[interface->current_altsetting];
                if (altsetting->set_altsetting)
                    altsetting->set_altsetting(data, intfid, interface->current_altsetting);
                break;
            }
            default: break;
            }
            break;
            default: break;
        }
        break;
        case USB_SETUP_BMREQUESTTYPE_RECIPIENT_ENDPOINT:
        {
            if (!data->state->current_configuration) break;
            union usb_endpoint_number ep = { .byte = buffer->setup.wIndex };
            int intfid;
            int epid;
            const struct usb_endpoint* endpoint = usb_find_endpoint(data, ep, &intfid, &epid);
            if (!endpoint) break;
            if (endpoint->ctrl_request) size = endpoint->ctrl_request(data, intfid, epid, buffer, &addr);
            if (size != -1) break;
            switch (buffer->setup.bmRequestType.type)
            {
            case USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD:
                switch (buffer->setup.bRequest.req)
                {
                case USB_SETUP_BREQUEST_GET_STATUS:
                    if (buffer->setup.wLength != 2 || buffer->setup.wValue) break;
                    data->buffer->raw[0] = 0;
                    data->buffer->raw[1] = data->driver->get_stall(data, ep);
                    addr = data->buffer;
                    size = 2;
                    break;
                case USB_SETUP_BREQUEST_CLEAR_FEATURE:
                    if (buffer->setup.wLength || buffer->setup.wValue) break;
                    data->driver->set_stall(data, ep, false);
                    size = 0;
                    break;
                case USB_SETUP_BREQUEST_SET_FEATURE:
                    if (buffer->setup.wLength || buffer->setup.wValue) break;
                    data->driver->set_stall(data, ep, true);
                    size = 0;
                    break;
                default: break;
                }
                break;
                default: break;
            }
            break;
        }
        default: break;
    }
    }
    // See comment at the top of this function
    if (size == 0) usb_ep0_start_tx(data, NULL, 0);
    else if (size > 0) usb_ep0_start_tx(data, addr, size);
    else if (size >= -2) usb_ep0_expect_setup(data);
}

static void usb_handle_ep0_data(const struct usb_instance* data, union usb_ep0_buffer* buffer, int size)
{
    // We don't accept any commands that have an OUT payload. Just STALL it, no matter what it was.
    usb_ep0_expect_setup(data);
}

void usb_handle_bus_reset(const struct usb_instance* data, int highspeed)
{
    data->state->current_address = 0;
    usb_unconfigure(data);
    if (data->bus_reset) data->bus_reset(data, highspeed);
    int c, i;
    for (c = 0; c < data->configuration_count; c++)
    {
        const struct usb_configuration* configuration = data->configurations[c];
        for (i = 0; i < configuration->interface_count; i++)
        {
            struct usb_interface* interface = configuration->interfaces[i];
            if (interface->bus_reset) interface->bus_reset(data, c, i, highspeed);
        }
    }
}

void usb_handle_timeout(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft)
{
    // Hm, the host didn't fetch our EP0 IN packet, so we feel a bit offended.
    // Refuse to accept any transfers, until we get a new SETUP token.
    if (!epnum.number) usb_ep0_expect_setup(data);
    else
    {
        int epidx;
        int ifidx;
        const struct usb_endpoint* endpoint = usb_find_endpoint(data, epnum, &epidx, &ifidx);
        if (!endpoint) data->driver->unconfigure_ep(data, epnum);
        else if (endpoint->timeout) endpoint->timeout(data, ifidx, epidx, bytesleft);
    }
}

void usb_handle_xfer_complete(const struct usb_instance* data, union usb_endpoint_number epnum, int bytesleft)
{
    if (!epnum.number)
    {
        // If this was EP0 IN, stall the pipe. There will only ever be one transfer for each
        // SETUP transaction, and the next SETUP packet will clear the stall.
        if (epnum.direction == USB_ENDPOINT_DIRECTION_IN) usb_set_stall(data, epnum, 1);
        else if (!data->ep0_data_hook || !data->ep0_data_hook(data, data->buffer, 64 - bytesleft))
            usb_handle_ep0_data(data, data->buffer, 64 - bytesleft);
    }
    else
    {
        int epidx;
        int ifidx;
        const struct usb_endpoint* endpoint = usb_find_endpoint(data, epnum, &epidx, &ifidx);
        if (!endpoint) usb_unconfigure_ep(data, epnum);
        else if (endpoint->xfer_complete) endpoint->xfer_complete(data, ifidx, epidx, bytesleft);
    }
}

void usb_handle_setup_received(const struct usb_instance* data, union usb_endpoint_number epnum, int back2back)
{
    if (!epnum.number)
    {
        // Figure out the location of the newest SETUP packet if there were
        // multiple back to back ones. If there were more than 3 SETUP packets
        // in a row, the OTG will take care of it, so we're on the safe side here.
        void* addr = &data->buffer->raw[8 * (back2back - 1)];
        union usb_ep0_buffer* buffer = (union usb_ep0_buffer*)addr;
        if (!data->ep0_setup_hook || !data->ep0_setup_hook(data, buffer)) usb_handle_ep0_setup(data, buffer);
    }
    else
    {
        int epidx;
        int ifidx;
        const struct usb_endpoint* endpoint = usb_find_endpoint(data, epnum, &epidx, &ifidx);
        if (!endpoint) usb_unconfigure_ep(data, epnum);
        else if (endpoint->setup_received) endpoint->setup_received(data, ifidx, epidx, back2back);
    }
}

void usb_init(const struct usb_instance* data)
{
    // Initialize data struct
    data->state->current_address = 0;
    data->state->current_configuration = 0;

    // Initialize driver
    data->driver->init(data);
}

void usb_exit(const struct usb_instance* data)
{
    // Shut down driver
    data->driver->exit(data);

    // Unconfigure everything
    usb_unconfigure(data);
}

