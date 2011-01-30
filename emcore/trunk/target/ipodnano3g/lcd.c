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
#include "thread.h"
#include "s5l8702.h"
#include "util.h"


static struct dma_lli lcd_lli[(LCD_WIDTH * LCD_HEIGHT - 1) / 0xfff]
    IDATA_ATTR __attribute__((aligned(16)));

static uint16_t lcd_color IDATA_ATTR;

static struct mutex lcd_mutex;


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

int lcd_get_format()
{
    return LCD_FORMAT;
}

static void lcd_send_cmd(uint16_t cmd) ICODE_ATTR __attribute__((noinline));
static void lcd_send_cmd(uint16_t cmd)
{
    while (LCDSTATUS & 0x10);
    LCDWCMD = cmd;
}

static void lcd_send_data(uint16_t data) ICODE_ATTR __attribute__((noinline));
static void lcd_send_data(uint16_t data)
{
    while (LCDSTATUS & 0x10);
    LCDWDATA = (data & 0xff) | ((data & 0x7f00) << 1);
}

static uint32_t lcd_detect() ICODE_ATTR;
static uint32_t lcd_detect()
{
    return (PDAT6 & 0x30) >> 4;
}

bool displaylcd_busy() ICODE_ATTR;
bool displaylcd_busy()
{
    return DMAC0C4CONFIG & 1;
}

bool displaylcd_safe()
{
    return !(DMAC0C4CONFIG & 1);
}

void displaylcd_sync() ICODE_ATTR;
void displaylcd_sync()
{
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
    while (displaylcd_busy()) sleep(100);
    mutex_unlock(&lcd_mutex);
}

void displaylcd_setup(unsigned int startx, unsigned int endx,
                      unsigned int starty, unsigned int endy) ICODE_ATTR;
void displaylcd_setup(unsigned int startx, unsigned int endx,
                      unsigned int starty, unsigned int endy)
{
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
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
}

static void displaylcd_dma(void* data, int pixels, bool solid) ICODE_ATTR;
static void displaylcd_dma(void* data, int pixels, bool solid)
{
    int i;
    for (i = -1; i < (int)ARRAYLEN(lcd_lli) && pixels > 0; i++, pixels -= 0xfff)
    {
        bool last = i + 1 >= ARRAYLEN(lcd_lli) || pixels <= 0xfff;
        struct dma_lli* lli = i < 0 ? (struct dma_lli*)((int)&DMAC0C4LLI) : &lcd_lli[i];
        lli->srcaddr = data;
        lli->dstaddr = (void*)((int)&LCDWDATA);
        lli->nextlli = last ? NULL : &lcd_lli[i + 1];
        lli->control = 0x70240000 | (last ? pixels : 0xfff)
                     | (last ? 0x80000000 : 0) | (solid ? 0 : 0x4000000);
        data = (void*)(((uint32_t)data) + 0x1ffe);
    }
    clean_dcache();
    DMAC0C4CONFIG = 0x88c1;
}

void displaylcd_native(unsigned int startx, unsigned int endx,
                       unsigned int starty, unsigned int endy, void* data)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    displaylcd_setup(startx, endx, starty, endy);
    displaylcd_dma(data, pixels, false);
    mutex_unlock(&lcd_mutex);
}

void filllcd_native(unsigned int startx, unsigned int endx,
                    unsigned int starty, unsigned int endy, int color)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    displaylcd_setup(startx, endx, starty, endy);
    lcd_color = color;
    displaylcd_dma(&lcd_color, pixels, true);
    mutex_unlock(&lcd_mutex);
}

void displaylcd_dither(unsigned int x, unsigned int y, unsigned int width,
                       unsigned int height, void* data, unsigned int datax,
                       unsigned int datay, unsigned int stride, bool solid) ICODE_ATTR;
void displaylcd_dither(unsigned int x, unsigned int y, unsigned int width,
                       unsigned int height, void* data, unsigned int datax,
                       unsigned int datay, unsigned int stride, bool solid)
{
    int pixels = width * height;
    if (pixels <= 0) return;
    displaylcd_setup(x, x + width - 1, y, y + height - 1);
    int corrsize = width * 3;
    signed char* corr = (signed char*)malloc(corrsize);
    if (!corr)
    {
        mutex_unlock(&lcd_mutex);
        return;
    }
    memset(corr, 0, corrsize);
    unsigned char* in = (unsigned char*)data + (stride * datay + datax) * 3;
    for (y = 0; y < height; y++)
    {
        int i;
        signed char* corrptr = corr;
        signed char lastcorr[3] = {0};
        for (x = 0; x < width; x++)
        {
            unsigned int pixel = 0;
            signed char* lastcorrptr = lastcorr;
            int orig = *in++ + *corrptr + *lastcorrptr;
            orig = MAX(0, MIN(255, orig));
            unsigned int real = orig >> 3;
            pixel |= real << 11;
            int err = orig - ((real << 3) | (real >> 2));
            *corrptr++ = (*lastcorrptr >> 1) + err >> 2;
            *lastcorrptr++ = err >> 1;
            orig = *in++ + *corrptr + *lastcorrptr;
            orig = MAX(0, MIN(255, orig));
            real = orig >> 2;
            pixel |= real << 5;
            err = orig - ((real << 2) | (real >> 4));
            *corrptr++ = (*lastcorrptr >> 1) + err >> 2;
            *lastcorrptr++ = err >> 1;
            orig = *in++ + *corrptr + *lastcorrptr;
            orig = MAX(0, MIN(255, orig));
            real = orig >> 3;
            pixel |= real;
            err = orig - ((real << 3) | (real >> 2));
            *corrptr++ = (*lastcorrptr >> 1) + err >> 2;
            *lastcorrptr++ = err >> 1;
            while (LCDSTATUS & 0x10);
            LCDWDATA = pixel;
            if (solid) in -= 3;
        }
        if (solid) in += stride * 3;
        else in += (stride - width) * 3;
    }
    free(corr);
    mutex_unlock(&lcd_mutex);
}

void displaylcd(unsigned int x, unsigned int y, unsigned int width, unsigned int height,
                void* data, unsigned int datax, unsigned int datay, unsigned int stride)
{
    displaylcd_dither(x, y, width, height, data, datax, datay, stride, false);
}

void filllcd(unsigned int x, unsigned int y, unsigned int width, unsigned int height, int color)
{
    if (width * height <= 0) return;
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
    lcd_color = color;
    displaylcd_dither(x, y, width, height, &lcd_color, 0, 0, 0, true);
    mutex_unlock(&lcd_mutex);
}

void lcd_shutdown()
{
    displaylcd_sync();
    uint32_t type = lcd_detect();
    if (type & 2)
    {
        lcd_send_cmd(0x7);
        lcd_send_data(0x172);
        lcd_send_cmd(0x30);
        lcd_send_data(0x3ff);
        sleep(90000);
        lcd_send_cmd(0x7);
        lcd_send_data(0x120);
        lcd_send_cmd(0x30);
        lcd_send_data(0x0);
        lcd_send_cmd(0x100);
        lcd_send_data(0x780);
        lcd_send_cmd(0x7);
        lcd_send_data(0x0);
        lcd_send_cmd(0x101);
        lcd_send_data(0x260);
        lcd_send_cmd(0x102);
        lcd_send_data(0xa9);
        sleep(30000);
        lcd_send_cmd(0x100);
        lcd_send_data(0x700);
        lcd_send_cmd(0x100);
        lcd_send_data(0x704);
    }
    else if (type == 1)
    {
        lcd_send_cmd(0x28);
        lcd_send_cmd(0x10);
        sleep(100000);
    }
    else
    {
        lcd_send_cmd(0x28);
        sleep(50000);
        lcd_send_cmd(0x10);
        sleep(50000);
    }
}

void INT_DMAC0C4()
{
    DMAC0INTTCCLR = 0x10;
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
