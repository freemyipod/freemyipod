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


#ifndef __LCDCONSOLE_H__
#define __LCDCONSOLE_H__


#include "global.h"
#include "drawing.h"
#include "lcd.h"


#define LCDCONSOLE_COLS (FRAMEBUF_WIDTH / FONT_WIDTH)
#define LCDCONSOLE_ROWS (FRAMEBUF_HEIGHT / FONT_HEIGHT)
#define LCDCONSOLE_OFFSETX ((FRAMEBUF_WIDTH - LCDCONSOLE_COLS * FONT_WIDTH) / 2)
#define LCDCONSOLE_OFFSETY ((FRAMEBUF_HEIGHT - LCDCONSOLE_ROWS * FONT_HEIGHT) / 2)


void lcdconsole_init();
void lcdconsole_putc(char string, int fgcolor, int bgcolor) ICODE_ATTR;
void lcdconsole_puts(const char* string, int fgcolor, int bgcolor) ICODE_ATTR;
void lcdconsole_write(const char* string, size_t length, int fgcolor, int bgcolor) ICODE_ATTR;
void lcdconsole_putc_noblit(char string, int fgcolor, int bgcolor) ICODE_ATTR;
void lcdconsole_puts_noblit(const char* string, int fgcolor, int bgcolor) ICODE_ATTR;
void lcdconsole_write_noblit(const char* string, size_t length, int fgcolor, int bgcolor) ICODE_ATTR;
void lcdconsole_update() ICODE_ATTR;
void lcdconsole_callback() ICODE_ATTR;
int lcdconsole_get_current_x() ICODE_ATTR;
int lcdconsole_get_current_y() ICODE_ATTR;
int lcdconsole_get_lineend_x() ICODE_ATTR;
int lcdconsole_get_lineend_y() ICODE_ATTR;


#endif
