//
//
//    Copyright 2011 TheSeven
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


#include "global.h"
#include "thread.h"
#include "util.h"
#include "lcd.h"


static struct mutex lcd_mutex IDATA_ATTR;
#define fb ((uint32_t*)0x0fc00000)


void lcd_init()
{
    mutex_init(&lcd_mutex);
}

int lcd_get_width()
{
    return LCD_WIDTH;
}

int lcd_get_height()
{
    return LCD_HEIGHT;
}

int lcd_get_bytes_per_pixel()
{
    return LCD_BYTESPERPIXEL;
}

int lcd_get_format()
{
    return LCD_FORMAT;
}

void lcd_shutdown()
{
}

bool displaylcd_busy() ICODE_ATTR;
bool displaylcd_busy()
{
    return false;
}

void displaylcd_sync() ICODE_ATTR;
void displaylcd_sync()
{
}

void displaylcd_native(unsigned int startx, unsigned int endx,
                       unsigned int starty, unsigned int endy, void* data)
{
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
    displaylcd_safe_native(startx, endx, starty, endy, data);
    mutex_unlock(&lcd_mutex);
}

void displaylcd_safe_native(unsigned int startx, unsigned int endx,
                            unsigned int starty, unsigned int endy, void* data)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    uint32_t* ptr = &fb[starty * LCD_WIDTH + startx];
    int rows = endy - starty + 1;
    int rowsize = (endx - startx + 1) * 4;
    while (rows--)
    {
        memcpy(ptr, data, rowsize);
        ptr += LCD_WIDTH;
        data += rowsize;
    }
}

void filllcd_native(unsigned int startx, unsigned int endx,
                    unsigned int starty, unsigned int endy, int color)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    uint32_t* ptr = &fb[starty * LCD_WIDTH + startx];
    int rows = endy - starty + 1;
    int rowsize = (endx - startx + 1) * 4;
    while (rows--)
    {
        int i;
        for (i = 0; i < rowsize; i += 4) fb[i] = color;
        ptr += LCD_WIDTH;
    }
    mutex_unlock(&lcd_mutex);
}

void displaylcd(unsigned int x, unsigned int y, unsigned int width, unsigned int height,
                void* data, unsigned int datax, unsigned int datay, unsigned int stride)
{
    if (width * height <= 0) return;
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
    uint32_t* ptr = &fb[y * LCD_WIDTH + x];
    uint8_t* inptr = &((uint8_t*)data)[datay * stride + datax];
    int rowsize = stride * 3;
    while (height--)
    {
        int pixels = width;
        uint8_t* ip = inptr;
        uint32_t* op = ptr;
        while (pixels--)
        {
            uint32_t value = *ip++ << 16;
            value |= *ip++ << 8;
            value |= *ip++;
            *op++ = value;
        }
        inptr += rowsize;
        ptr += LCD_WIDTH;
    }
    mutex_unlock(&lcd_mutex);
}

void filllcd(unsigned int x, unsigned int y, unsigned int width, unsigned int height, int color)
{
    filllcd_native(x, y, width, height, color);
}

int lcd_translate_color(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
    ICODE_ATTR __attribute__((naked, noinline));
int lcd_translate_color(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
    asm volatile(
        "orr r0, r3, r0,lsl#24 \n\t"
        "orr r1, r2, r1,lsl#8  \n\t"
        "orr r0, r0, r1,lsl#8  \n\t"
        "mov pc, lr            \n\t"
    );
}
