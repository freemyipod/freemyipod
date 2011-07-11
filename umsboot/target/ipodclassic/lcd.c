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


#include "global.h"
#include "s5l8702.h"
#include "util.h"


static struct dma_lli lcd_lli[(LCD_WIDTH * LCD_HEIGHT - 1) / 0xfff]
    IDATA_ATTR CACHEALIGN_ATTR;

static uint16_t lcd_color IDATA_ATTR;


void lcd_init()
{
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

static void lcd_send_cmd(uint16_t cmd)
{
    while (LCDSTATUS & 0x10);
    LCDWCMD = cmd;
}

static void lcd_send_data(uint16_t data)
{
    while (LCDSTATUS & 0x10);
    LCDWDATA = (data & 0xff) | ((data & 0x7f00) << 1);
}

static uint32_t lcd_detect()
{
    return (PDAT6 & 0x30) >> 4;
}

void lcd_shutdown()
{
}

bool displaylcd_busy()
{
    return DMAC0C0CONFIG & 1;
}

bool displaylcd_safe()
{
    return !(DMAC0C0CONFIG & 1);
}

void displaylcd_sync()
{
    while (displaylcd_busy());
}

void displaylcd(unsigned int startx, unsigned int endx,
                unsigned int starty, unsigned int endy, void* data, int color)
{
    displaylcd_sync();
    if (lcd_detect() & 2)
    {
        lcd_send_cmd(0x210);
        lcd_send_data(startx);
        lcd_send_cmd(0x211);
        lcd_send_data(endx);
        lcd_send_cmd(0x212);
        lcd_send_data(starty);
        lcd_send_cmd(0x213);
        lcd_send_data(endy);
        lcd_send_cmd(0x200);
        lcd_send_data(startx);
        lcd_send_cmd(0x201);
        lcd_send_data(starty);
        lcd_send_cmd(0x202);
    }
    else
    {
        lcd_send_cmd(0x2a);
        lcd_send_data(startx >> 8);
        lcd_send_data(startx & 0xff);
        lcd_send_data(endx >> 8);
        lcd_send_data(endx & 0xff);
        lcd_send_cmd(0x2b);
        lcd_send_data(starty >> 8);
        lcd_send_data(starty & 0xff);
        lcd_send_data(endy >> 8);
        lcd_send_data(endy & 0xff);
        lcd_send_cmd(0x2c);
    }
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    int i;
    bool solid = (int)data == -1;
    if (solid) lcd_color = color;
    for (i = -1; i < (int)ARRAYLEN(lcd_lli) && pixels > 0; i++, pixels -= 0xfff)
    {
        bool last = i + 1 >= ARRAYLEN(lcd_lli) || pixels <= 0xfff;
        struct dma_lli* lli = i < 0 ? (struct dma_lli*)((int)&DMAC0C0LLI) : &lcd_lli[i];
        lli->srcaddr = solid ? &lcd_color : data;
        lli->dstaddr = (void*)((int)&LCDWDATA);
        lli->nextlli = last ? NULL : &lcd_lli[i + 1];
        lli->control = 0x70240000 | (last ? pixels : 0xfff)
                     | (last ? 0x80000000 : 0) | (solid ? 0 : 0x4000000);
        data = (void*)(((uint32_t)data) + 0x1ffe);
    }
    clean_dcache();
    DMAC0C0CONFIG = 0x88c1;
}

void INT_DMAC0C0()
{
    DMAC0INTTCCLR = 1;
    lcdconsole_callback();
}

int lcd_translate_color(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
    ICODE_ATTR __attribute__((naked, noinline));
int lcd_translate_color(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
    asm volatile(
        "cmp r0, #0xff             \n\t"
        "moveq r0, #-1             \n\t"
        "moveq pc, lr              \n\t"
        "cmp r0, #0                \n\t"
        "movne r0, #0xff000000     \n\t"
        "orrne r0, r0, #0xff0000   \n\t"
        "mov r2, r2,lsr#2          \n\t"
        "orr r0, r0, r3,lsr#3      \n\t"
        "mov r1, r1,lsr#3          \n\t"
        "orr r0, r0, r2,lsl#5      \n\t"
        "orr r0, r0, r1,lsl#11     \n\t"
        "mov pc, lr                \n\t"
    );
}
