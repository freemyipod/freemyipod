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


#include "global.h"
#include "lcdconsole.h"
#include "util.h"


#define OFFSETX LCDCONSOLE_OFFSETX
#define OFFSETY LCDCONSOLE_OFFSETY
#define PIXELBYTES (LCD_BYTESPERPIXEL)
#define LINEBYTES (LCD_WIDTH * PIXELBYTES)
#define COLBYTES (FONT_WIDTH * PIXELBYTES)
#define ROWBYTES (FONT_HEIGHT * LINEBYTES)
#define OFFSETBYTES (LINEBYTES * OFFSETY + PIXELBYTES * OFFSETX)


static unsigned char framebuf[LCD_FRAMEBUFSIZE];
static unsigned int current_row;
static unsigned int current_col;


void lcdconsole_init()
{
  memset(framebuf, -1, sizeof(framebuf));
  current_row = 0;
  current_col = -1;
}

void lcdconsole_putc(char string, int fgcolor, int bgcolor)
{
  if (string == '\r') return;
  current_col++;
  if (string == '\n')
  {
    current_col = -1;
    current_row++;
    return;
  }
  if (string == '\t')
  {
    current_col |= 3;
    return;
  }
  if (current_col >= LCDCONSOLE_COLS)
  {
    current_col = 0;
    current_row++;
  }
  if (current_row >= LCDCONSOLE_ROWS)
  {
    int offset = current_row - LCDCONSOLE_ROWS + 1;
    memcpy(framebuf, &framebuf[ROWBYTES * offset], ROWBYTES * offset);
    memset(&framebuf[sizeof(framebuf) - ROWBYTES * offset],
           -1, ROWBYTES * offset);
  }
  renderchar(&framebuf[OFFSETBYTES + ROWBYTES * current_row
                     + COLBYTES * current_col],
             fgcolor, bgcolor, string, LINEBYTES);
}

void lcdconsole_puts(const char* string, int fgcolor, int bgcolor)
{
  while (*string) lcdconsole_putc(*string++, fgcolor, bgcolor);
}

void lcdconsole_update()
{
  displaylcd(0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1, framebuf, 0);
}
