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


#ifndef __USBDRV_H__
#define __USBDRV_H__


#include "global.h"


int usb_drv_port_speed(void);
int usb_drv_request_endpoint(int type, int dir);
void usb_drv_release_endpoint(int ep);
void usb_drv_set_address(int address);
int usb_drv_send(int endpoint, const void *ptr, int length);
int usb_drv_send_nonblocking(int endpoint, const void *ptr, int length);
int usb_drv_recv(int endpoint, void* ptr, int length);
void usb_drv_cancel_all_transfers(void);
bool usb_drv_stalled(int endpoint, bool in);
void usb_drv_stall(int endpoint, bool stall, bool in);
void usb_drv_init(void) INITCODE_ATTR;
void usb_drv_exit(void);
int usb_drv_get_max_out_size();
int usb_drv_get_max_in_size();


#endif
