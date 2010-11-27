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


#ifndef __TARGET_H__
#define __TARGET_H__


#define PLATFORM_ID 0x47334e49


#define ARM_ARCH 5
#define LITTLE_ENDIAN
#define CACHEALIGN_BITS 4
#define CPU_FREQ 191692800


#define RAMDISK_SECTORS 32768
#define RAMDISK_SECTORSIZE 2048


#define HAVE_USB
#define USB_NUM_ENDPOINTS 5

#define HAVE_LCD
#define HAVE_LCD_SHUTDOWN
#define LCD_WIDTH 320
#define LCD_HEIGHT 240
#define LCD_FORMAT rgb565
#define LCD_BYTESPERPIXEL 2
#define FRAMEBUF_WIDTH 176
#define FRAMEBUF_HEIGHT 98

#define HAVE_BACKLIGHT

#define HAVE_I2C


#endif
