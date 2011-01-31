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


#ifndef __DRAWING_H__
#define __DRAWING_H__


#include "global.h"


#define FONT_WIDTH 6
#define FONT_HEIGHT 8


void renderchar_native(void* buffer, int fgcol, int bgcol, char text, int stride);
void renderchar(void* buffer, int x, int y, int stride,
                uint32_t fgcolor, uint32_t bgcolor, char c);
void rendertext(void* buffer, int x, int y, int stride,
                uint32_t fgcolor, uint32_t bgcolor, char* str);
int get_font_width(void);
int get_font_height(void);


#endif
