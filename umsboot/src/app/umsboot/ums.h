#ifndef __APP_UMSBOOT_UMS_H__
#define __APP_UMSBOOT_UMS_H__

#include "global.h"
#include "protocol/usb/usb.h"


extern volatile bool ums_ejected;


extern void ums_bus_reset(const struct usb_instance* data, int configuration, int interface, int highspeed);
extern int ums_ctrl_request(const struct usb_instance* data, int interface, union usb_ep0_buffer* request, const void** response);
extern void ums_set_altsetting(const struct usb_instance* data, int interface, int altsetting);
extern void ums_unset_altsetting(const struct usb_instance* data, int interface, int altsetting);
extern int ums_ep_ctrl_request(const struct usb_instance* data, int interface, int endpoint, union usb_ep0_buffer* request, const void** response);
extern void ums_xfer_complete(const struct usb_instance* data, int interface, int endpoint, int bytesleft);
extern void ums_timeout(const struct usb_instance* data, int interface, int endpoint, int bytesleft);


#endif
