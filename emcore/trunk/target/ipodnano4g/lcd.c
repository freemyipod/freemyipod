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
#include "s5l8720.h"
#include "util.h"
#include "clockgates-target.h"
#include "lcd.h"


static struct dma_lli lcd_lli[(LCD_WIDTH * LCD_HEIGHT - 1) / 0xfff]
    IDATA_ATTR CACHEALIGN_ATTR;

static uint16_t lcd_color IDATA_ATTR;

static struct mutex lcd_mutex IDATA_ATTR;
static struct wakeup lcd_wakeup IDATA_ATTR;

static bool lcd_dma_busy IDATA_ATTR;


void lcd_init()
{
    mutex_init(&lcd_mutex);
    wakeup_init(&lcd_wakeup);
    lcd_dma_busy = true;
    clockgate_dma(0, 4, true);
    if (!(DMAC0C4CONFIG & 1))
    {
        lcd_dma_busy = false;
        clockgate_dma(0, 4, false);
    }
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

void lcd_shutdown()
{
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
    displaylcd_sync();
    while (!(LCDSTATUS & 0x2));
    LCDCON = 0x41100db8;
}

bool displaylcd_busy() ICODE_ATTR;
bool displaylcd_busy()
{
    return lcd_dma_busy;
}

void displaylcd_sync() ICODE_ATTR;
void displaylcd_sync()
{
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
    while (displaylcd_busy()) wakeup_wait(&lcd_wakeup, TIMEOUT_BLOCK);
    mutex_unlock(&lcd_mutex);
}

void displaylcd_setup(unsigned int startx, unsigned int endx,
                      unsigned int starty, unsigned int endy, bool safe) ICODE_ATTR;
void displaylcd_setup(unsigned int startx, unsigned int endx,
                      unsigned int starty, unsigned int endy, bool safe)
{
    if (!safe)
    {
        mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
        displaylcd_sync();
    }
    else while (DMAC0C4CONFIG & 1);
    while (!(LCDSTATUS & 0x2));
    LCDCON = 0x41100db8;
    lcd_send_cmd(0x2a);
    lcd_send_data(startx);
    lcd_send_data(endx);
    lcd_send_cmd(0x2b);
    lcd_send_data(starty);
    lcd_send_data(endy);
    lcd_send_cmd(0x2c);
}

static void displaylcd_dma(void* data, int pixels, bool solid) ICODE_ATTR;
static void displaylcd_dma(void* data, int pixels, bool solid)
{
    int i;
    lcd_dma_busy = true;
    clockgate_dma(0, 4, true);
    for (i = -1; i < (int)ARRAYLEN(lcd_lli) && pixels > 0; i++, pixels -= 0xfff)
    {
        bool last = i + 1 >= ARRAYLEN(lcd_lli) || pixels <= 0xfff;
        struct dma_lli* lli = i < 0 ? (struct dma_lli*)((int)&DMAC0C4LLI) : &lcd_lli[i];
        lli->srcaddr = data;
        lli->dstaddr = (void*)((int)&LCDWDATA);
        lli->nextlli = last ? NULL : &lcd_lli[i + 1];
        lli->control = 0x70240000 | (last ? pixels : 0xfff)
                     | (last ? 0x80000000 : 0) | (solid ? 0 : 0x4000000);
        if (!solid) data += 0x1ffe;
    }
    clean_dcache();
    DMAC0C4CONFIG = 0x88c1;
}

void displaylcd_native(unsigned int startx, unsigned int endx,
                       unsigned int starty, unsigned int endy, void* data)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    displaylcd_setup(startx, endx, starty, endy, false);
    displaylcd_dma(data, pixels, false);
    mutex_unlock(&lcd_mutex);
}

void displaylcd_safe_native(unsigned int startx, unsigned int endx,
                            unsigned int starty, unsigned int endy, void* data)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    displaylcd_setup(startx, endx, starty, endy, true);
    displaylcd_dma(data, pixels, false);
}

void filllcd_native(unsigned int startx, unsigned int endx,
                    unsigned int starty, unsigned int endy, int color)
{
    int pixels = (endx - startx + 1) * (endy - starty + 1);
    if (pixels <= 0) return;
    displaylcd_setup(startx, endx, starty, endy, false);
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
//TODO: This is ARMv5E optimized assembly, should be converted to ARMv6
    __asm__ volatile("    muls r12, r2, r3             \n");
    __asm__ volatile("    bxeq lr                      \n");
    __asm__ volatile("    stmfd sp!, {r1-r11,lr}       \n");
    __asm__ volatile("    mov r12, #0                  \n");
    __asm__ volatile("    str r12, [sp]                \n");
    __asm__ volatile("    mov r12, r2                  \n");
    __asm__ volatile("    add r8, r2, r2,lsl#1         \n");
    __asm__ volatile("    add r3, r1, r3               \n");
    __asm__ volatile("    sub r3, r3, #1               \n");
    __asm__ volatile("    mov r2, r1                   \n");
    __asm__ volatile("    add r1, r0, r12              \n");
    __asm__ volatile("    sub r1, r1, #1               \n");
    __asm__ volatile("    bl displaylcd_setup          \n");
    __asm__ volatile("    add sp, sp, #4               \n");
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
    __asm__ volatile("displaylcd_dither_wait :         \n");
    __asm__ volatile("    ldr r4, [r9,#-0x24]          \n");
    __asm__ volatile("    tst r4, #2                   \n");
    __asm__ volatile("    beq displaylcd_dither_wait   \n");
    __asm__ volatile("    ldr r4, =0x41104eb8          \n");
    __asm__ volatile("    str r4, [r9,#-0x40]          \n");
    __asm__ volatile("displaylcd_dither_y:             \n");
    __asm__ volatile("    ldr lr, [sp]                 \n");
    __asm__ volatile("    mov r4, #0                   \n");
    __asm__ volatile("    mov r5, #0                   \n");
    __asm__ volatile("    mov r6, #0                   \n");
    __asm__ volatile("    mov r7, r8                   \n");
    __asm__ volatile("displaylcd_dither_x:             \n");
    __asm__ volatile("    ldrb r1, [r3], #1            \n");
    __asm__ volatile("    ldrsb r0, [r7]               \n");
    __asm__ volatile("    add r1, r1, r4               \n");
    __asm__ volatile("    add r1, r1, r0               \n");
    __asm__ volatile("    cmp r1, #0xff                \n");
    __asm__ volatile("    mvnhi r1, r1,asr#31          \n");
    __asm__ volatile("    andhi r1, r1, #0xff          \n");
    __asm__ volatile("    mov r0, r1,lsr#2             \n");
    __asm__ volatile("    mov r2, r0,lsl#18            \n");
    __asm__ volatile("    sub r1, r1, r0,lsl#2         \n");
    __asm__ volatile("    sub r1, r1, r0,lsr#4         \n");
    __asm__ volatile("    mov r4, r4,lsr#1             \n");
    __asm__ volatile("    add r4, r4, r1,lsr#2         \n");
    __asm__ volatile("    strb r4, [r7], #1            \n");
    __asm__ volatile("    mov r4, r1,asr#1             \n");
    __asm__ volatile("    ldrb r1, [r3], #1            \n");
    __asm__ volatile("    ldrsb r0, [r7]               \n");
    __asm__ volatile("    add r1, r1, r5               \n");
    __asm__ volatile("    add r1, r1, r0               \n");
    __asm__ volatile("    cmp r1, #0xff                \n");
    __asm__ volatile("    mvnhi r1, r1,asr#31          \n");
    __asm__ volatile("    andhi r1, r1, #0xff          \n");
    __asm__ volatile("    mov r0, r1,lsr#2             \n");
    __asm__ volatile("    orr r2, r2, r0,lsl#10        \n");
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
    __asm__ volatile("    cmp r1, #0xff                \n");
    __asm__ volatile("    mvnhi r1, r1,asr#31          \n");
    __asm__ volatile("    andhi r1, r1, #0xff          \n");
    __asm__ volatile("    mov r0, r1,lsr#2             \n");
    __asm__ volatile("    orr r2, r2, r0,lsl#2         \n");
    __asm__ volatile("    sub r1, r1, r0,lsl#2         \n");
    __asm__ volatile("    sub r1, r1, r0,lsr#4         \n");
    __asm__ volatile("    mov r6, r6,lsr#1             \n");
    __asm__ volatile("    add r6, r6, r1,lsr#2         \n");
    __asm__ volatile("    strb r6, [r7], #1            \n");
    __asm__ volatile("displaylcd_dither_wait2:         \n");
    __asm__ volatile("    ldr r0, [r9,#-0x24]          \n");
    __asm__ volatile("    mov r6, r1,asr#1             \n");
    __asm__ volatile("    tst r0, #0x10                \n");
    __asm__ volatile("    bne displaylcd_dither_wait2  \n");
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
    displaylcd_sync();
    lcd_color = color;
    displaylcd_dither(x, y, width, height, &lcd_color, 0, 0, 0, true);
    mutex_unlock(&lcd_mutex);
}

void INT_DMAC0C4()
{
    DMAC0INTTCCLR = 0x10;
    lcd_dma_busy = false;
    clockgate_dma(0, 4, false);
    lcdconsole_callback();
    wakeup_signal(&lcd_wakeup);
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
