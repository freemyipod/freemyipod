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
#include "s5l8701.h"
#include "util.h"


static struct mutex lcd_mutex;


void lcd_init()
{
    DMACON8 = 0x20590000;
    LCDCON = 0xd01;
    LCDPHTIME = 0;
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
    LCDWDATA = data;
}

static uint32_t lcd_detect() ICODE_ATTR;
static uint32_t lcd_detect()
{
    return (PDAT13 & 1) | (PDAT14 & 2);
}

bool displaylcd_busy() ICODE_ATTR;
bool displaylcd_busy()
{
    return DMAALLST2 & 0x70000;
}

bool displaylcd_safe()
{
    return !(DMAALLST2 & 0x70000);
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
    if (lcd_detect() == 2)
    {
        lcd_send_cmd(0x50);
        lcd_send_data(startx);
        lcd_send_cmd(0x51);
        lcd_send_data(endx);
        lcd_send_cmd(0x52);
        lcd_send_data(starty);
        lcd_send_cmd(0x53);
        lcd_send_data(endy);
        lcd_send_cmd(0x20);
        lcd_send_data(startx);
        lcd_send_cmd(0x21);
        lcd_send_data(starty);
        lcd_send_cmd(0x22);
    }
    else
    {
        lcd_send_cmd(0x2a);
        lcd_send_data(startx);
        lcd_send_data(endx);
        lcd_send_cmd(0x2b);
        lcd_send_data(starty);
        lcd_send_data(endy);
        lcd_send_cmd(0x2c);
    }
}

static void displaylcd_dma(void* data, int pixels) ICODE_ATTR;
static void displaylcd_dma(void* data, int pixels)
{
    uint16_t* in = (uint16_t*)data;
    while (LCDSTATUS & 8);
    while (pixels & 3)
	{
        LCDWDATA = *in++;
        pixels--;
	}
    DMABASE8 = in;
    DMACON8 = 0x20590000;
    DMATCNT8 = pixels / 4;
    clean_dcache();
    DMACOM8 = 4;
}

static void displaylcd_solid(uint16_t data, int pixels) ICODE_ATTR;
static void displaylcd_solid(uint16_t data, int pixels)
{
    while (pixels >= 4)
    {
        while (LCDSTATUS & 8);
        LCDWDATA = data;
        LCDWDATA = data;
        LCDWDATA = data;
        LCDWDATA = data;
        pixels -= 4;
    }
    while (LCDSTATUS & 8);
    while (pixels & 3)
	{
        LCDWDATA = data;
        pixels--;
	}
}

void displaylcd_native(unsigned int startx, unsigned int endx,
                       unsigned int starty, unsigned int endy, void* data)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    displaylcd_setup(startx, endx, starty, endy);
    displaylcd_dma(data, pixels);
    mutex_unlock(&lcd_mutex);
}

void filllcd_native(unsigned int startx, unsigned int endx,
                    unsigned int starty, unsigned int endy, int color)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    displaylcd_setup(startx, endx, starty, endy);
    displaylcd_solid(color, pixels);
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
    displaylcd_dither(x, y, width, height, &color, 0, 0, 0, true);
    mutex_unlock(&lcd_mutex);
}

void lcd_shutdown()
{
    displaylcd_sync();
    uint32_t type = lcd_detect();
    if (type == 2)
    {
        lcd_send_cmd(0x7);
        lcd_send_data(0x232);
        lcd_send_cmd(0x13);
        lcd_send_data(0x1137);
        lcd_send_cmd(0x7);
        lcd_send_data(0x201);
        lcd_send_cmd(0x13);
        lcd_send_data(0x137);
        lcd_send_cmd(0x7);
        lcd_send_data(0x200);
        lcd_send_cmd(0x10);
        lcd_send_data(0x680);
        lcd_send_cmd(0x12);
        lcd_send_data(0x160);
        lcd_send_cmd(0x13);
        lcd_send_data(0x127);
        lcd_send_cmd(0x10);
        lcd_send_data(0x600);
    }
    else
    {
        lcd_send_cmd(0x28);
        lcd_send_cmd(0x10);
    }
    sleep(5000);
}

void INT_DMA8()
{
    DMACOM8 = 7;
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
