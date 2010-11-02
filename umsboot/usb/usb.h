//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __USB_H__
#define __USB_H__


#include "global.h"
#include "usb_ch9.h"


extern bool usb_ejected;

void usb_handle_control_request(struct usb_ctrlrequest* req);
void usb_handle_transfer_complete(int endpoint, int dir, int status, int length);
void usb_handle_bus_reset(void);
void usb_init(void);
void usb_exit(void);


#endif
