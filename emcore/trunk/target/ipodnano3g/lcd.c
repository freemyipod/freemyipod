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
                       unsigned int datay, unsigned int stride, bool solid)
     ICODE_ATTR __attribute__((naked,noinline));
void displaylcd_dither(unsigned int x, unsigned int y, unsigned int width,
                       unsigned int height, void* data, unsigned int datax,
                       unsigned int datay, unsigned int stride, bool solid)
{
    __asm__ volatile("    muls r12, r2, r3             \n");
    __asm__ volatile("    bxeq lr                      \n");
    __asm__ volatile("    stmfd sp!, {r2-r11,lr}       \n");
    __asm__ volatile("    mov r12, r2                  \n");
    __asm__ volatile("    add r8, r2, r2,lsl#1         \n");
    __asm__ volatile("    add r3, r1, r3               \n");
    __asm__ volatile("    sub r3, r3, #1               \n");
    __asm__ volatile("    mov r2, r1                   \n");
    __asm__ volatile("    add r1, r0, r12              \n");
    __asm__ volatile("    sub r1, r1, #1               \n");
    __asm__ volatile("    bl displaylcd_setup          \n");
    __asm__ volatile("    mov r0, r8                   \n");
    __asm__ volatile("    bl malloc                    \n");
    __asm__ volatile("    cmp r0, #0                   \n");
    __asm__ volatile("    beq displaylcd_dither_unlock \n");
    __asm__ volatile("    mov r2, r8                   \n");
    __asm__ volatile("    mov r1, #0                   \n");
    __asm__ volatile("    mov r8, r0                   \n");
    __asm__ volatile("    bl memset                    \n");
    __asm__ volatile("    ldr r0, [sp,#0x30]           \n");
    __asm__ volatile("    ldr r1, [sp,#0x34]           \n");
    __asm__ volatile("    ldr r11, [sp,#0x38]          \n");
    __asm__ volatile("    ldr r3, [sp,#0x2c]           \n");
    __asm__ volatile("    mla r0, r1, r11, r0          \n");
    __asm__ volatile("    ldr r12, [sp,#0x04]          \n");
    __asm__ volatile("    ldr r2, [sp,#0x3c]           \n");
    __asm__ volatile("    add r3, r3, r0,lsl#1         \n");
    __asm__ volatile("    cmp r2, #0                   \n");
    __asm__ volatile("    ldreq r1, [sp]               \n");
    __asm__ volatile("    add r3, r3, r0               \n");
    __asm__ volatile("    subeq r11, r11, r1           \n");
    __asm__ volatile("    add r11, r11, r11,lsl#1      \n");
    __asm__ volatile("    movne r10, #3                \n");
    __asm__ volatile("    moveq r10, #0                \n");
    __asm__ volatile("    ldr r9, =0x38300040          \n");
    __asm__ volatile("displaylcd_dither_y:             \n");
    __asm__ volatile("    ldr lr, [sp]                 \n");
    __asm__ volatile("    mov r4, #0                   \n");
    __asm__ volatile("    mov r5, #0                   \n");
    __asm__ volatile("    mov r6, #0                   \n");
    __asm__ volatile("    mov r7, r8                   \n");
    __asm__ volatile("displaylcd_dither_x:             \n");
    __asm__ volatile("    mov r2, #0                   \n");
    __asm__ volatile("    ldrb r1, [r3], #1            \n");
    __asm__ volatile("    ldrsb r0, [r7]               \n");
    __asm__ volatile("    add r1, r1, r4               \n");
    __asm__ volatile("    add r1, r1, r0               \n");
    __asm__ volatile("    cmp r1, #0                   \n");
    __asm__ volatile("    movlt r1, #0                 \n");
    __asm__ volatile("    cmp r1, #0xff                \n");
    __asm__ volatile("    movgt r1, #0xff              \n");
    __asm__ volatile("    mov r0, r1,lsr#3             \n");
    __asm__ volatile("    orr r2, r0,lsl#11            \n");
    __asm__ volatile("    sub r1, r1, r0,lsl#3         \n");
    __asm__ volatile("    sub r1, r1, r0,lsr#2         \n");
    __asm__ volatile("    mov r4, r4,lsr#1             \n");
    __asm__ volatile("    add r4, r4, r1,lsr#2         \n");
    __asm__ volatile("    strb r4, [r7], #1            \n");
    __asm__ volatile("    mov r4, r1,asr#1             \n");
    __asm__ volatile("    ldrb r1, [r3], #1            \n");
    __asm__ volatile("    ldrsb r0, [r7]               \n");
    __asm__ volatile("    add r1, r1, r5               \n");
    __asm__ volatile("    add r1, r1, r0               \n");
    __asm__ volatile("    cmp r1, #0                   \n");
    __asm__ volatile("    movlt r1, #0                 \n");
    __asm__ volatile("    cmp r1, #0xff                \n");
    __asm__ volatile("    movgt r1, #0xff              \n");
    __asm__ volatile("    mov r0, r1,lsr#2             \n");
    __asm__ volatile("    orr r2, r0,lsl#5             \n");
    __asm__ volatile("    sub r1, r1, r0,lsl#2         \n");
    __asm__ volatile("    sub r1, r1, r0,lsr#4         \n");
    __asm__ volatile("    mov r5, r5,lsr#1             \n");
    __asm__ volatile("    add r5, r5, r1,lsr#2         \n");
    __asm__ volatile("    strb r5, [r7], #1            \n");
    __asm__ volatile("    mov r5, r1,asr#1             \n");
    __asm__ volatile("    ldrb r1, [r3], #1            \n");
    __asm__ volatile("    ldrsb r0, [r7]               \n");
    __asm__ volatile("    add r1, r1, r6               \n");
    __asm__ volatile("    add r1, r1, r0               \n");
    __asm__ volatile("    cmp r1, #0                   \n");
    __asm__ volatile("    movlt r1, #0                 \n");
    __asm__ volatile("    cmp r1, #0xff                \n");
    __asm__ volatile("    movgt r1, #0xff              \n");
    __asm__ volatile("    mov r0, r1,lsr#3             \n");
    __asm__ volatile("    orr r2, r0                   \n");
    __asm__ volatile("    sub r1, r1, r0,lsl#3         \n");
    __asm__ volatile("    sub r1, r1, r0,lsr#2         \n");
    __asm__ volatile("    mov r6, r6,lsr#1             \n");
    __asm__ volatile("    add r6, r6, r1,lsr#2         \n");
    __asm__ volatile("    strb r6, [r7], #1            \n");
    __asm__ volatile("    mov r6, r1,asr#1             \n");
    __asm__ volatile("    str r2, [r9]                 \n");
    __asm__ volatile("    sub r3, r3, r10              \n");
    __asm__ volatile("    subs lr, lr, #1              \n");
    __asm__ volatile("    bne displaylcd_dither_x      \n");
    __asm__ volatile("    add r3, r3, r11              \n");
    __asm__ volatile("    subs r12, r12, #1            \n");
    __asm__ volatile("    bne displaylcd_dither_y      \n");
    __asm__ volatile("displaylcd_dither_free:          \n");
    __asm__ volatile("    mov r0, r8                   \n");
    __asm__ volatile("    bl free                      \n");
    __asm__ volatile("displaylcd_dither_unlock:        \n");
    __asm__ volatile("    ldr r0, =lcd_mutex           \n");
    __asm__ volatile("    bl mutex_unlock              \n");
    __asm__ volatile("    ldmfd sp!, {r2-r11,pc}       \n");
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
