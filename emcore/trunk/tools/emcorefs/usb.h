//
//
//    Copyright 2011 user890104
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

#include "global.h"


int usb_init(void);
int usb_find(uint16_t vendor_id, uint16_t product_id, uint8_t* reattach);
int usb_open(libusb_device* dev, uint8_t* reattach);
int usb_bulk_transfer(unsigned char endpoint, void* data, int length);
int usb_close(uint8_t reattach);
void usb_exit(void);
int usb_destroy(uint8_t reattach);

#endif /* __USB_H__ */
