//
//
//    Copyright 2010 TheSeven
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


#ifndef __USBDEBUG_H__
#define __USBDEBUG_H__


#include "../global.h"
#include "usb.h"


extern void usbdebug_enable(const struct usb_instance* data, int interface, int altsetting);
extern void usbdebug_disable(const struct usb_instance* data, int interface, int altsetting);
extern void usbdebug_bulk_enable(const struct usb_instance* data, int interface, int altsetting);
extern void usbdebug_bulk_disable(const struct usb_instance* data, int interface, int altsetting);
extern int usbdebug_handle_setup(const struct usb_instance* data, int interface, union usb_ep0_buffer* request, const void** response);
extern void usbdebug_bulk_xfer_complete(const struct usb_instance* data, int interface, int endpoint, int bytesleft);
extern int usbdebug_bulk_ctrl_request(const struct usb_instance* data, int interface, int endpoint, union usb_ep0_buffer* request, const void** response);
extern void usbdebug_bus_reset(const struct usb_instance* data, int configuration, int interface, int highspeed);
extern void dbgconsole_putc(char string) ICODE_ATTR;
extern void dbgconsole_puts(const char* string) ICODE_ATTR;
extern void dbgconsole_write(const char* string, size_t length) ICODE_ATTR;
extern void dbgconsole_sputc(char string) ICODE_ATTR;
extern void dbgconsole_sputs(const char* string) ICODE_ATTR;
extern void dbgconsole_swrite(const char* string, size_t length) ICODE_ATTR;
extern int dbgconsole_getc(int timeout) ICODE_ATTR;
extern int dbgconsole_read(char* string, size_t length, int timeout) ICODE_ATTR;


#endif
