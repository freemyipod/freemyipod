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


#ifndef __USB_H__
#define __USB_H__

#include "emcoreapp.h"


extern int usb_maxlen;


extern void usb_prepare();
extern void usb_connect();
extern void enqueue_async();
extern void usb_transmit(void* buffer, uint32_t len);
extern void usb_receive(void* buffer, uint32_t len);
extern void usb_stall();

#endif

