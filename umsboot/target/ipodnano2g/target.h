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


#ifndef __TARGET_H__
#define __TARGET_H__


#define PLATFORM_ID 0x47324e49


#define ARM_ARCH 4
#define LITTLE_ENDIAN
#define CACHEALIGN_BITS 4
#define CPU_FREQ 191692800


#define RAMDISK_SECTORS 16384
#define RAMDISK_SECTORSIZE 2048


#define HAVE_USB
#define USB_NUM_ENDPOINTS 5

#define HAVE_LCD
#define HAVE_LCD_SHUTDOWN
#define LCD_WIDTH 176
#define LCD_HEIGHT 132
#define LCD_FORMAT rgb565
#define LCD_BYTESPERPIXEL 2
#define FRAMEBUF_WIDTH 176
#define FRAMEBUF_HEIGHT 132
#define LCDCONSOLE_FGCOLOR 0
#define LCDCONSOLE_BGCOLOR -1

#define HAVE_BACKLIGHT

#define HAVE_I2C


#endif
