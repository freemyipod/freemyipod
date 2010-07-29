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


#ifndef __LCD_H__
#define __LCD_H__


#include "global.h"


#define LCD_WIDTH 240
#define LCD_HEIGHT 320
#define LCD_FORMAT rgb565
#define LCD_BYTESPERPIXEL 2
#define LCD_FRAMEBUFSIZE (LCD_WIDTH * LCD_HEIGHT * LCD_BYTESPERPIXEL)


void displaylcd(unsigned int startx, unsigned int endx,
                unsigned int starty, unsigned int endy, void* data, int color);
void displaylcd_sync();


#endif
