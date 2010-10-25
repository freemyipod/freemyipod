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


#define PLATFORM_ID 0x47324e49


#define ARM_ARCH 4
#define LITTLE_ENDIAN
#define CACHEALIGN_BITS 4
#define CPU_FREQ 191692800


#define CONSOLE_BOOT 3


#define HAVE_USB
#define USB_NUM_ENDPOINTS 5
#define USB_HAVE_TARGET_SPECIFIC_REQUESTS

#define HAVE_LCD
#define HAVE_LCD_SHUTDOWN
#define LCD_WIDTH 176
#define LCD_HEIGHT 132
#define LCD_FORMAT rgb565
#define LCD_BYTESPERPIXEL 2

#define HAVE_BACKLIGHT

#define HAVE_I2C

#define HAVE_HWKEYAES

#define HAVE_HMACSHA1

#define HAVE_BUTTON
#define HAVE_CLICKWHEEL

#define HAVE_BOOTFLASH
#define BOOTFLASH_IS_MEMMAPPED

#define HAVE_STORAGE
#define HAVE_FLASH_STORAGE
#define HAVE_STORAGE_FLUSH
#define HAVE_HOTSWAP
#define CONFIG_STORAGE STORAGE_NAND
#define SECTOR_SIZE 2048

#define HAVE_TARGETINIT_LATE


#endif
