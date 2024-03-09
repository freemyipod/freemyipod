#ifndef __USBGLUE_H__
#define __USBGLUE_H__

#include "../global.h"
#include "usb.h"

extern void usbmanager_init();
extern void usbmanager_exit();
extern int usbmanager_install_custom(const struct usb_devicedescriptor* devicedescriptor,
                                     uint8_t config_count, const struct usb_configuration** configurations,
                                     uint8_t string_count, const struct usb_stringdescriptor** stringdescriptors,
                                     bool enable_debug);
extern void usbmanager_uninstall_custom();
extern uint32_t usbmanager_get_available_endpoints();
extern bool usbmanager_get_connected();

#endif
