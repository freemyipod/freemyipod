//
//
//    Copyright 2011 user890104
//
//
//    This file is part of ipoddfu.
//
//    ipoddfu is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    ipoddfu is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with ipoddfu.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __USB_H__
#define __USB_H__

int usb_init(void);
int usb_find(unsigned char *reattach);
int usb_open(libusb_device *dev, unsigned char *reattach);
int usb_bulk_transfer(const unsigned char endpoint, unsigned char *data, const unsigned int length);
int usb_control_transfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength);
int usb_close(const unsigned char reattach);
void usb_exit(void);

#endif
