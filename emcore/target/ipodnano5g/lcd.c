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

/*
 * iPod nano 5G LCD driver, adapted from the nano 4G LCD driver.
 *
 * Controller 0x38300000, command-mode panel with its own GRAM, 240 x 376 RGB565 (red in 15..11,
 * bl_Gop:0x2e6). Nothing initialises the panel on the DFU path (the ROM has no 0x38300000 reference), so lcd_init()
 * replays DP_OnLcdProtocol bl_DisplayPlatform:0x3f0: LCD clock + gates, LCDCON = 0x01100db8, LCDPHTIME = 0x11, panel
 * detection (DCS 0x04, 3 bytes after a dummy read, bl_DisplayPlatform:0x220, 5 tries), then a native C
 * initialization routine for the detected panel. Command values and delays follow bl_DisplayPlatform;
 * original file offsets are listed below. Initialization optionally fills the panel white by PIO.
 * Pixel transport: DMAC0 channel 4 -> LCDWDATA, halfword, CONFIG 0x88c1, request line 3 (bl_DmaLight:0x2c0/0x554,
 * fw_disk:0x0807175c), LLI chain of 0xfff-transfer blocks as the 4G, CONTROL = 0x00240000 | n | SI, TC on the last.
 * Every wait is bounded (the ROM/DFU harness has no other diagnostic channel than USB): STATUS waits ~10 ms, the
 * read-ready wait 1 ms, DMA 100 ms (4G DMA timeout guard). Counters in lcd_panel_info are host-readable (READ_MEM at the
 * symbol address in emcore-nano5g.nm).
 */

#include "global.h"
#include "thread.h"
#include "s5l8730.h"
#include "util.h"
#include "lcd.h"
#include "timer.h"
#include "contextswitch.h"
#include "clockgates.h"
#include "mmu.h"
#include "lcdconsole.h"
#include "console.h"
#include "module5g.h"


static struct dma_lli lcd_lli[(LCD_WIDTH * LCD_HEIGHT - 1) / 0xfff]
    IDATA_ATTR CACHEALIGN_ATTR;

static uint16_t lcd_color IDATA_ATTR;

static struct mutex lcd_mutex IDATA_ATTR;
static struct wakeup lcd_wakeup IDATA_ATTR;

static bool lcd_dma_busy IDATA_ATTR;

/* DMA timeout guard: a frame takes a few ms; a channel "busy" for this long is stopped. */
#define LCD_DMA_TIMEOUT_US 100000
#define LCD_STATUS_TIMEOUT_US 10000
static uint32_t lcd_dma_start IDATA_ATTR;
uint32_t lcd_dma_aborts;              /* number of F01 recoveries (host-readable) */

/* Panel identification, filled by lcd_init. Host-readable (symbol lcd_panel_info in emcore-nano5g.nm):
   magic 'LCDP', id[0..3] = the 4 reads after DCS 0x04 (id[0] = dummy, id[1..3] = Apple's 3 ID bytes),
   type = index into lcd_panels[] or -1 (unknown: nothing was sent), tries = detection rounds used, ran = 1 when the
   initialization routine completed, status_timeouts / read_timeouts = bounded waits that expired, lcdcon0 = LCDCON as found at
   entry (reset value expected after a ROM DFU boot), clkcon2 = CLKCON2 read-back after the write. */
struct lcd_panel_info
{
    uint32_t magic;
    uint8_t id[4];
    int8_t type;
    uint8_t tries;
    uint8_t ran;
    uint8_t clk_ok;
    uint32_t status_timeouts;
    uint32_t read_timeouts;
    uint32_t lcdcon0;
    uint32_t clkcon2;
};
struct lcd_panel_info lcd_panel_info;

static void lcd_panel_init(void) INITCODE_ATTR;

/* One console line from lcd_panel_info (targetinit_late; lcd_init runs before the scheduler, too early to print). */
void lcd_print_panel_info(unsigned int consoles)
{
    static const char* const names[5] = { "38b3", "38c4", "38e6", "38f7", "48c4" };
    struct lcd_panel_info* p = &lcd_panel_info;
    cprintf(consoles, "[lcd] id %02X %02X %02X %02X panel %s tries %d init %d clk %d to %d/%d dmaab %d\n",
            p->id[0], p->id[1], p->id[2], p->id[3], p->type >= 0 ? names[(int)p->type] : "unknown",
            p->tries, p->ran, p->clk_ok, p->status_timeouts, p->read_timeouts, lcd_dma_aborts);
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

/* bl_Lcd:0x5b4 / 0x5d4: wait STATUS b4 (TX busy) clear, then write */
static bool lcd_wait_tx(void) ICODE_ATTR;
static bool lcd_wait_tx(void)
{
    long start = USEC_TIMER;
    while (LCDSTATUS & 0x10)
        if (TIMEOUT_EXPIRED(start, LCD_STATUS_TIMEOUT_US)) { lcd_panel_info.status_timeouts++; return false; }
    return true;
}

/* bl_DisplayPlatform:0x51c / 0x46c: wait STATUS b1 (interface idle) set, Stall(1 us), write LCDCON */
static void lcd_set_mode(uint32_t mode) ICODE_ATTR;
static void lcd_set_mode(uint32_t mode)
{
    long start = USEC_TIMER;
    while (!(LCDSTATUS & 0x2))
        if (TIMEOUT_EXPIRED(start, LCD_STATUS_TIMEOUT_US)) { lcd_panel_info.status_timeouts++; break; }
    udelay(1);
    LCDCON = mode;
}

static void lcd_send_cmd(uint8_t cmd) ICODE_ATTR __attribute__((noinline));
static void lcd_send_cmd(uint8_t cmd)
{
    lcd_wait_tx();
    LCDWCMD = cmd;
}

/* command data: one byte per +0x40 write, no 9-bit packing (bl_Lcd:0x5d4, bl_DisplayPlatform:0x4bc) */
static void lcd_send_data8(uint8_t data) ICODE_ATTR __attribute__((noinline));
static void lcd_send_data8(uint8_t data)
{
    lcd_wait_tx();
    LCDWDATA = data;
}

/* bl_Lcd:0x2f8: wait TX idle, LCDRDATA = 0, wait STATUS b0 (bounded; fw_disk 0x0803d718 bounds it too), byte = DBUFF >> 1 */
static bool lcd_read8(uint8_t* data) INITCODE_ATTR;
static bool lcd_read8(uint8_t* data)
{
    long start;
    if (!lcd_wait_tx()) return false;
    LCDRDATA = 0;
    start = USEC_TIMER;
    while (!(LCDSTATUS & 0x1))
        if (TIMEOUT_EXPIRED(start, 1000)) { lcd_panel_info.read_timeouts++; return false; }
    *data = LCDDBUFF >> 1;
    return true;
}

/* Panel-specific command values follow the bootloader DisplayPlatform sequences.
 * Each routine issues native C calls; parameter arrays contain only LCD register data.
 * The source offsets below identify the original sequences.
 */
static void lcd_fill_white(void) INITCODE_ATTR;

/* Restore pixel mode after every command, including commands without parameters. */
static void lcd_write_command(uint8_t cmd, const uint8_t* data, unsigned int count)
{
    lcd_set_mode(LCD_MODE_CMD);
    lcd_send_cmd(cmd);
    while (count--) lcd_send_data8(*data++);
    lcd_set_mode(LCD_MODE_PIXEL);
}

/* Infer the parameter count at compile time; there are no script opcodes or lengths. */
#define LCD_WRITE_DATA(cmd, ...) do { \
    static const uint8_t data[] = { __VA_ARGS__ }; \
    lcd_write_command((cmd), data, sizeof(data)); \
} while (0)

/* 38c4 init: DisplayPlatform file+0x584..0x64f */
static void lcd_38c4_init(bool with_fill)
{
    /* Exit sleep */
    lcd_write_command(0x11, NULL, 0);
    udelay(120000);
    /* Memory access control */
    LCD_WRITE_DATA(0x36, 0x00);
    /* Pixel format */
    LCD_WRITE_DATA(0x3a, 0x66);
    /* Column address range */
    LCD_WRITE_DATA(0x2a, 0x00, 0x00, 0x00, 0xef);
    /* Page address range */
    LCD_WRITE_DATA(0x2b, 0x00, 0x00, 0x01, 0x8f);
    /* Tearing effect output */
    LCD_WRITE_DATA(0x35, 0x00);
    LCD_WRITE_DATA(0xb1, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb2, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb3, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb4, 0x02);
    LCD_WRITE_DATA(0xb6, 0x01, 0x02);
    LCD_WRITE_DATA(0xc0, 0x00);
    LCD_WRITE_DATA(0xc1, 0x04, 0x0b);
    LCD_WRITE_DATA(0xc2, 0x04, 0x03);
    LCD_WRITE_DATA(0xc3, 0x02, 0x05);
    LCD_WRITE_DATA(0xc4, 0x02, 0x04);
    LCD_WRITE_DATA(0xc5, 0x24, 0x24);
    LCD_WRITE_DATA(0xea, 0x01);
    LCD_WRITE_DATA(0xe0, 0x1d, 0x2b, 0x2e, 0x08, 0x12, 0x13, 0x2b, 0x9b,
                   0x35, 0x0c, 0x1c, 0x25, 0x24, 0x25, 0x2a);
    LCD_WRITE_DATA(0xe1, 0x00, 0x1f, 0x23, 0x09, 0x12, 0x13, 0x1c, 0x7a,
                   0x2d, 0x0d, 0x1f, 0x29, 0x2c, 0x2d, 0x33);
    LCD_WRITE_DATA(0xe2, 0x1d, 0x2b, 0x2f, 0x08, 0x12, 0x12, 0x2a, 0xab,
                   0x34, 0x0e, 0x20, 0x25, 0x22, 0x23, 0x2a);
    LCD_WRITE_DATA(0xe3, 0x00, 0x21, 0x25, 0x09, 0x0e, 0x11, 0x1d, 0x79,
                   0x2e, 0x0e, 0x1f, 0x29, 0x2c, 0x2d, 0x33);
    LCD_WRITE_DATA(0xe4, 0x23, 0x2f, 0x31, 0x09, 0x13, 0x14, 0x2b, 0x9b,
                   0x33, 0x0b, 0x1c, 0x24, 0x24, 0x25, 0x2a);
    LCD_WRITE_DATA(0xe5, 0x00, 0x1f, 0x23, 0x0a, 0x11, 0x13, 0x1e, 0x79,
                   0x2d, 0x11, 0x22, 0x2c, 0x24, 0x25, 0x25);
    if (with_fill) lcd_fill_white();
    /* Normal display mode */
    lcd_write_command(0x13, NULL, 0);
    /* Display on */
    lcd_write_command(0x29, NULL, 0);
}

/* 38c4 sleep: DisplayPlatform file+0x650..0x659 */
static void lcd_38c4_sleep(void)
{
    /* Display off */
    lcd_write_command(0x28, NULL, 0);
    /* Enter sleep */
    lcd_write_command(0x10, NULL, 0);
    udelay(120000);
}

/* 48c4 init: DisplayPlatform file+0x65a..0x71e */
static void lcd_48c4_init(bool with_fill)
{
    /* Exit sleep */
    lcd_write_command(0x11, NULL, 0);
    udelay(120000);
    LCD_WRITE_DATA(0xf0, 0x5a);
    LCD_WRITE_DATA(0xf1, 0x5a);
    LCD_WRITE_DATA(0xb0, 0x2a, 0x2a, 0x21, 0x21, 0x0a, 0x0a, 0x0a, 0x0a,
                   0x0a, 0x0a, 0x0a, 0x0a, 0xaf, 0x30, 0x00, 0x01, 0x02);
    LCD_WRITE_DATA(0xb1, 0xa0, 0x81, 0x18, 0x2a, 0x7f, 0x1a, 0x33, 0x5e);
    LCD_WRITE_DATA(0xb2, 0x00, 0x01, 0x03, 0x03, 0x0a, 0x0a);
    LCD_WRITE_DATA(0xb8, 0x48, 0x00, 0x00);
    LCD_WRITE_DATA(0xc0, 0x19, 0x11, 0x17, 0x21, 0x30, 0x39, 0x40, 0x4a,
                   0x4e, 0x4a, 0x4b, 0x51, 0x36);
    LCD_WRITE_DATA(0xc1, 0x08, 0x00, 0x03, 0x06, 0x0f, 0x15, 0x17, 0x1f,
                   0x26, 0x24, 0x26, 0x2d, 0x15);
    LCD_WRITE_DATA(0xc2, 0x17, 0x11, 0x1b, 0x27, 0x33, 0x3b, 0x40, 0x4a,
                   0x4e, 0x4a, 0x4b, 0x4f, 0x33);
    LCD_WRITE_DATA(0xc3, 0x07, 0x00, 0x06, 0x09, 0x11, 0x16, 0x18, 0x1f,
                   0x26, 0x25, 0x28, 0x2c, 0x13);
    LCD_WRITE_DATA(0xc4, 0x1c, 0x12, 0x19, 0x22, 0x30, 0x37, 0x3c, 0x45,
                   0x4a, 0x47, 0x4a, 0x51, 0x36);
    LCD_WRITE_DATA(0xc5, 0x0e, 0x00, 0x01, 0x02, 0x08, 0x0b, 0x0c, 0x14,
                   0x1c, 0x1c, 0x21, 0x29, 0x12);
    /* Pixel format */
    LCD_WRITE_DATA(0x3a, 0x06);
    LCD_WRITE_DATA(0xd0, 0x12);
    /* Column address range */
    LCD_WRITE_DATA(0x2a, 0x00, 0x00, 0x00, 0xef);
    /* Page address range */
    LCD_WRITE_DATA(0x2b, 0x00, 0x00, 0x01, 0x77);
    /* Tearing effect output */
    LCD_WRITE_DATA(0x35, 0x00);
    LCD_WRITE_DATA(0xf0, 0xa5);
    LCD_WRITE_DATA(0xf1, 0xa5);
    if (with_fill) lcd_fill_white();
    /* Normal display mode */
    lcd_write_command(0x13, NULL, 0);
    /* Display on */
    lcd_write_command(0x29, NULL, 0);
}

/* 48c4 sleep: DisplayPlatform file+0x71f..0x728 */
static void lcd_48c4_sleep(void)
{
    /* Display off */
    lcd_write_command(0x28, NULL, 0);
    /* Enter sleep */
    lcd_write_command(0x10, NULL, 0);
    udelay(120000);
}

/* 38e6 init: DisplayPlatform file+0x729..0x7f8 */
static void lcd_38e6_init(bool with_fill)
{
    /* Exit sleep */
    lcd_write_command(0x11, NULL, 0);
    udelay(120000);
    /* Tearing effect output */
    LCD_WRITE_DATA(0x35, 0x00);
    /* Memory access control */
    LCD_WRITE_DATA(0x36, 0x00);
    /* Pixel format */
    LCD_WRITE_DATA(0x3a, 0x66);
    /* Column address range */
    LCD_WRITE_DATA(0x2a, 0x00, 0x00, 0x00, 0xef);
    /* Page address range */
    LCD_WRITE_DATA(0x2b, 0x00, 0x00, 0x01, 0x8f);
    LCD_WRITE_DATA(0xb1, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb2, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb3, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb4, 0x02);
    LCD_WRITE_DATA(0xb6, 0x21, 0x00);
    LCD_WRITE_DATA(0xc0, 0x20);
    LCD_WRITE_DATA(0xc1, 0x01, 0x0b);
    LCD_WRITE_DATA(0xc2, 0x04, 0x03);
    LCD_WRITE_DATA(0xc3, 0x04, 0x03);
    LCD_WRITE_DATA(0xc4, 0x04, 0x03);
    LCD_WRITE_DATA(0xc5, 0x33, 0x36);
    LCD_WRITE_DATA(0xea, 0x01);
    LCD_WRITE_DATA(0xeb, 0x00);
    LCD_WRITE_DATA(0xe0, 0x0e, 0x14, 0x19, 0x06, 0x0d, 0x11, 0x1a, 0x8a,
                   0x2b, 0x0e, 0x22, 0x27, 0x1e, 0x20, 0x22);
    LCD_WRITE_DATA(0xe1, 0x1d, 0x1f, 0x21, 0x08, 0x0d, 0x11, 0x14, 0x57,
                   0x25, 0x0e, 0x22, 0x29, 0x26, 0x2b, 0x31);
    LCD_WRITE_DATA(0xe2, 0x0e, 0x1b, 0x1f, 0x05, 0x0c, 0x10, 0x1b, 0x89,
                   0x2a, 0x0e, 0x21, 0x27, 0x1e, 0x20, 0x22);
    LCD_WRITE_DATA(0xe3, 0x1d, 0x1f, 0x21, 0x08, 0x0e, 0x11, 0x15, 0x67,
                   0x24, 0x0f, 0x23, 0x2a, 0x20, 0x24, 0x31);
    LCD_WRITE_DATA(0xe4, 0x0e, 0x18, 0x1b, 0x05, 0x0c, 0x10, 0x17, 0x89,
                   0x26, 0x0d, 0x21, 0x27, 0x1a, 0x1d, 0x22);
    LCD_WRITE_DATA(0xe5, 0x1d, 0x22, 0x25, 0x08, 0x0e, 0x12, 0x19, 0x67,
                   0x28, 0x0f, 0x23, 0x2a, 0x24, 0x27, 0x31);
    if (with_fill) lcd_fill_white();
    /* Normal display mode */
    lcd_write_command(0x13, NULL, 0);
    /* Display on */
    lcd_write_command(0x29, NULL, 0);
}

/* 38e6 sleep: DisplayPlatform file+0x7f9..0x804 */
static void lcd_38e6_sleep(void)
{
    /* Display off */
    lcd_write_command(0x28, NULL, 0);
    udelay(50000);
    /* Enter sleep */
    lcd_write_command(0x10, NULL, 0);
    udelay(120000);
}

/* 38b3 init: DisplayPlatform file+0x805..0x8d4 */
static void lcd_38b3_init(bool with_fill)
{
    /* Exit sleep */
    lcd_write_command(0x11, NULL, 0);
    udelay(120000);
    /* Tearing effect output */
    LCD_WRITE_DATA(0x35, 0x00);
    /* Memory access control */
    LCD_WRITE_DATA(0x36, 0x00);
    /* Pixel format */
    LCD_WRITE_DATA(0x3a, 0x66);
    /* Column address range */
    LCD_WRITE_DATA(0x2a, 0x00, 0x00, 0x00, 0xef);
    /* Page address range */
    LCD_WRITE_DATA(0x2b, 0x00, 0x00, 0x01, 0x8f);
    LCD_WRITE_DATA(0xb1, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb2, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb3, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb4, 0x02);
    LCD_WRITE_DATA(0xb6, 0x21, 0x00);
    LCD_WRITE_DATA(0xc0, 0x20);
    LCD_WRITE_DATA(0xc1, 0x01, 0x0b);
    LCD_WRITE_DATA(0xc2, 0x04, 0x03);
    LCD_WRITE_DATA(0xc3, 0x04, 0x03);
    LCD_WRITE_DATA(0xc4, 0x04, 0x03);
    LCD_WRITE_DATA(0xc5, 0x33, 0x36);
    LCD_WRITE_DATA(0xea, 0x01);
    LCD_WRITE_DATA(0xeb, 0x00);
    LCD_WRITE_DATA(0xe0, 0x0b, 0x13, 0x16, 0x07, 0x0d, 0x10, 0x19, 0x88,
                   0x2a, 0x0e, 0x21, 0x28, 0x1d, 0x1f, 0x21);
    LCD_WRITE_DATA(0xe1, 0x1e, 0x20, 0x22, 0x07, 0x0e, 0x11, 0x15, 0x77,
                   0x26, 0x0f, 0x22, 0x28, 0x29, 0x2c, 0x34);
    LCD_WRITE_DATA(0xe2, 0x0b, 0x18, 0x1c, 0x06, 0x0c, 0x0f, 0x19, 0x79,
                   0x29, 0x0e, 0x22, 0x2a, 0x1c, 0x1f, 0x21);
    LCD_WRITE_DATA(0xe3, 0x1e, 0x20, 0x23, 0x05, 0x0d, 0x11, 0x16, 0x68,
                   0x26, 0x10, 0x23, 0x29, 0x23, 0x27, 0x34);
    LCD_WRITE_DATA(0xe4, 0x0b, 0x14, 0x17, 0x06, 0x0c, 0x10, 0x15, 0x78,
                   0x25, 0x0b, 0x1d, 0x25, 0x1a, 0x1c, 0x21);
    LCD_WRITE_DATA(0xe5, 0x1e, 0x23, 0x25, 0x0a, 0x12, 0x14, 0x1a, 0x78,
                   0x2a, 0x0f, 0x23, 0x29, 0x28, 0x2b, 0x34);
    if (with_fill) lcd_fill_white();
    /* Normal display mode */
    lcd_write_command(0x13, NULL, 0);
    /* Display on */
    lcd_write_command(0x29, NULL, 0);
}

/* 38b3 sleep: DisplayPlatform file+0x8d5..0x8e0 */
static void lcd_38b3_sleep(void)
{
    /* Display off */
    lcd_write_command(0x28, NULL, 0);
    udelay(50000);
    /* Enter sleep */
    lcd_write_command(0x10, NULL, 0);
    udelay(120000);
}

/* 38f7 init: DisplayPlatform file+0x8e1..0x9bc */
static void lcd_38f7_init(bool with_fill)
{
    /* Exit sleep */
    lcd_write_command(0x11, NULL, 0);
    udelay(120000);
    /* Tearing effect output */
    LCD_WRITE_DATA(0x35, 0x00);
    /* Memory access control */
    LCD_WRITE_DATA(0x36, 0x00);
    /* Pixel format */
    LCD_WRITE_DATA(0x3a, 0x66);
    /* Column address range */
    LCD_WRITE_DATA(0x2a, 0x00, 0x00, 0x00, 0xef);
    /* Page address range */
    LCD_WRITE_DATA(0x2b, 0x00, 0x00, 0x01, 0x77);
    LCD_WRITE_DATA(0xb1, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb2, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb3, 0x11, 0x1b);
    LCD_WRITE_DATA(0xb4, 0x02);
    LCD_WRITE_DATA(0xb6, 0x01, 0x02);
    LCD_WRITE_DATA(0xc0, 0x24);
    LCD_WRITE_DATA(0xc1, 0x02, 0x00);
    LCD_WRITE_DATA(0xc2, 0x05, 0x01);
    LCD_WRITE_DATA(0xc3, 0x02, 0x05);
    LCD_WRITE_DATA(0xc4, 0x02, 0x04);
    LCD_WRITE_DATA(0xc5, 0x14, 0x2e);
    LCD_WRITE_DATA(0xea, 0x01);
    LCD_WRITE_DATA(0xe0, 0x27, 0x2b, 0x2e, 0x06, 0x0d, 0x11, 0x28, 0x7b,
                   0x35, 0x0c, 0x20, 0x26, 0x25, 0x28, 0x3c);
    LCD_WRITE_DATA(0xe1, 0x08, 0x21, 0x26, 0x09, 0x0f, 0x12, 0x1f, 0x38,
                   0x31, 0x0d, 0x23, 0x29, 0x2c, 0x2f, 0x33);
    LCD_WRITE_DATA(0xe2, 0x27, 0x2c, 0x2f, 0x07, 0x0e, 0x11, 0x29, 0x7a,
                   0x35, 0x0c, 0x20, 0x26, 0x24, 0x29, 0x3c);
    LCD_WRITE_DATA(0xe3, 0x08, 0x20, 0x26, 0x09, 0x0f, 0x12, 0x1f, 0x48,
                   0x30, 0x0d, 0x22, 0x28, 0x2b, 0x2e, 0x33);
    LCD_WRITE_DATA(0xe4, 0x1f, 0x24, 0x27, 0x08, 0x0f, 0x12, 0x25, 0x7b,
                   0x32, 0x0c, 0x20, 0x26, 0x20, 0x25, 0x3c);
    LCD_WRITE_DATA(0xe5, 0x08, 0x24, 0x2b, 0x09, 0x0f, 0x12, 0x22, 0x38,
                   0x35, 0x0c, 0x21, 0x27, 0x33, 0x36, 0x3b);
    LCD_WRITE_DATA(0xf0, 0xaa, 0x55, 0x25);
    LCD_WRITE_DATA(0xf6, 0x7f, 0x8a, 0x34, 0x00, 0x04, 0x26, 0x07);
    if (with_fill) lcd_fill_white();
    /* Normal display mode */
    lcd_write_command(0x13, NULL, 0);
    /* Display on */
    lcd_write_command(0x29, NULL, 0);
}

/* 38f7 sleep: DisplayPlatform file+0x9bd..0x9ce */
static void lcd_38f7_sleep(void)
{
    /* Display off */
    lcd_write_command(0x28, NULL, 0);
    LCD_WRITE_DATA(0xf6, 0x7f, 0x8a, 0x34, 0x00, 0x04, 0x26, 0x00);
    /* Enter sleep */
    lcd_write_command(0x10, NULL, 0);
}

#undef LCD_WRITE_DATA

struct lcd_panel
{
    uint8_t id0, id1;
    void (*init)(bool with_fill);
    void (*sleep)(void);
};
/* Keep callbacks resident for shutdown and panel reinitialization after boot. */
static const struct lcd_panel lcd_panels[5] =
{
    { 0x38, 0xb3, lcd_38b3_init, lcd_38b3_sleep },
    { 0x38, 0xc4, lcd_38c4_init, lcd_38c4_sleep },
    { 0x38, 0xe6, lcd_38e6_init, lcd_38e6_sleep },
    { 0x38, 0xf7, lcd_38f7_init, lcd_38f7_sleep },
    { 0x48, 0xc4, lcd_48c4_init, lcd_48c4_sleep },
};

#define LCD_PANEL_MAGIC 0x5044434c                       /* 'LCDP' */

/* DP_SetWindow bl_DisplayPlatform:0x4bc: 2a/2b with four byte-wise coordinates, 2c; all in command mode */
static void lcd_window(unsigned int x0, unsigned int x1, unsigned int y0, unsigned int y1) ICODE_ATTR;
static void lcd_window(unsigned int x0, unsigned int x1, unsigned int y0, unsigned int y1)
{
    lcd_set_mode(LCD_MODE_CMD);
    lcd_send_cmd(0x2a);
    lcd_send_data8(x0 >> 8); lcd_send_data8(x0); lcd_send_data8(x1 >> 8); lcd_send_data8(x1);
    lcd_send_cmd(0x2b);
    lcd_send_data8(y0 >> 8); lcd_send_data8(y0); lcd_send_data8(y1 >> 8); lcd_send_data8(y1);
    lcd_send_cmd(0x2c);
    lcd_set_mode(LCD_MODE_PIXEL);
}

/* Initial white fill (bl_Lcd:0x422): SetWindow(0,0,240,376) then 2*w*h writes of 0xffff (Apple writes twice the pixel count; kept) */
static void lcd_fill_white(void)
{
    int n;
    lcd_window(0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);
    for (n = 0; n < 2 * LCD_WIDTH * LCD_HEIGHT; n++)
    {
        if (!lcd_wait_tx()) return;   /* a dead interface would cost 180k timeouts otherwise */
        LCDWDATA = 0xffff;
    }
}

/* Returns the index into lcd_panels or -1. Apple reads len+1 = 4 bytes and drops the first (bl_Lcd:0x2ce..0x2e0). */
static int lcd_detect_panel(void) INITCODE_ATTR;
static int lcd_detect_panel(void)
{
    int tries, i, type;
    for (tries = 1; tries <= 5; tries++)
    {
        lcd_panel_info.tries = tries;
        lcd_set_mode(LCD_MODE_CMD);
        lcd_send_cmd(0x04);
        for (i = 0; i < 4; i++)
            if (!lcd_read8(&lcd_panel_info.id[i])) break;
        lcd_set_mode(LCD_MODE_PIXEL);
        if (i == 4)
            for (type = 0; type < 5; type++)
                if (lcd_panel_info.id[1] == lcd_panels[type].id0 && lcd_panel_info.id[2] == lcd_panels[type].id1)
                    return type;
    }
    return -1;
}

/* DP_OnLcdProtocol bl_DisplayPlatform:0x3f0 */
static void lcd_panel_init(void)
{
    int type;
    long start;
    lcd_panel_info.magic = LCD_PANEL_MAGIC;
    lcd_panel_info.lcdcon0 = LCDCON;
    /* 1. gates: LCD = PWRCON0 b1 + PWRCON4 b16 (fw_disk gate id 0xf, 0x0802c5b4); DMAC0 = PWRCON0 b11 */
    clockgate_enable(CLOCKGATE_LCD, true);
    clockgate_enable(CLOCKGATE_LCD_2, true);
    clockgate_enable(CLOCKGATE_DMAC0, true);
    /* 2. CLKCON2 = 0x80001014, poll until it reads back (bl_DisplayPlatform:0x3f0, literal 0x454); bounded 100 ms */
    CLKCON2 = LCD_CLKCON2_VALUE;
    start = USEC_TIMER;
    while (CLKCON2 != LCD_CLKCON2_VALUE) if (TIMEOUT_EXPIRED(start, 100000)) break;
    lcd_panel_info.clkcon2 = CLKCON2;
    lcd_panel_info.clk_ok = lcd_panel_info.clkcon2 == LCD_CLKCON2_VALUE;
    /* 3. controller modes (fw_disk's +0x7c = 0x201 / +0x88 = 0x01000000 deliberately not sent) */
    LCDCON = LCD_MODE_PIXEL;
    LCDPHTIME = 0x11;
    /* 4. detect, 5. initialize (unknown panel: nothing is sent, the ID stays readable for the host) */
    type = lcd_detect_panel();
    lcd_panel_info.type = type;
    if (type >= 0)
    {
        lcd_panels[type].init(true);
        lcd_panel_info.ran = 1;
    }
    /* 6. leave pixel mode */
    lcd_set_mode(LCD_MODE_PIXEL);
}

void lcd_init()
{
    mutex_init(&lcd_mutex);
    wakeup_init(&lcd_wakeup);
    lcd_dma_busy = false;
    DMAC0CONFIG = 1;                     /* bl_DmaLight:0x2c0: DMACConfiguration = 1 */
    lcd_dma_start = USEC_TIMER;
    lcd_panel_init();
}

/* F01 recovery: stop channel 4 and drop the busy flag. Callable with IRQs on or off (panic path). */
static void lcd_dma_abort(void) ICODE_ATTR;
static void lcd_dma_abort(void)
{
    uint32_t mode = enter_critical_section();
    DMAC0C4CONFIG = 0;            /* PL080: clearing the enable bit stops the channel (queued data is lost) */
    DMAC0INTTCCLR = 0x10;
    DMAC0INTERRCLR = 0x10;
    lcd_dma_busy = false;
    lcd_dma_aborts++;
    clockgate_dma(0, 4, false);
    leave_critical_section(mode);
}

/* PowerState(1) bl_DisplayPlatform:0x490: put the panel to sleep without a white fill. */
void lcd_shutdown()
{
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
    displaylcd_sync();
    if (lcd_panel_info.type >= 0) lcd_panels[(int)lcd_panel_info.type].sleep();
    lcd_set_mode(LCD_MODE_PIXEL);
}

bool displaylcd_busy() ICODE_ATTR;
bool displaylcd_busy()
{
    if (lcd_dma_busy && TIMEOUT_EXPIRED(lcd_dma_start, LCD_DMA_TIMEOUT_US)) lcd_dma_abort();
    return lcd_dma_busy;
}

void displaylcd_sync() ICODE_ATTR;
void displaylcd_sync()
{
    mutex_lock(&lcd_mutex, TIMEOUT_BLOCK);
    while (displaylcd_busy()) wakeup_wait(&lcd_wakeup, LCD_DMA_TIMEOUT_US / 4);
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
    else
    {
        uint32_t start = USEC_TIMER;
        while (DMAC0C4CONFIG & 1)
            if (TIMEOUT_EXPIRED(start, LCD_DMA_TIMEOUT_US))
            {
                lcd_dma_abort();
                break;
            }
    }
    lcd_window(startx, endx, starty, endy);
}

/* bl_DmaLight:0x554 / fw_disk:0x0807175c: SRC = buffer, DST = LCDWDATA, CONTROL = 0x80240000 | SI | count (halfwords),
   CONFIG = 0x88c1 (dst request 3, mem->periph, ITC, enable); chunks of 0xfff transfers as the 4G LLI chain. */
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
        lli->control = 0x00240000 | (last ? pixels : 0xfff)
                     | (last ? 0x80000000 : 0) | (solid ? 0 : 0x4000000);
        if (!solid) data += 0x1ffe;
    }
    clean_dcache();               /* fw_disk:0x08071860 cleans the D-cache over the buffer first */
    lcd_dma_start = USEC_TIMER;
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

/* Generic blit of 32-bit BGRX pixels (emCORE apps, boot logo): converted to RGB565 and written by PIO in pixel mode,
   one halfword per +0x40 write (the 4G's dithering assembly relied on a 4G-only LCDCON mode 0x81104eb8 and is not
   ported). Slow but has no unknown register in it. */
void displaylcd(unsigned int x, unsigned int y, unsigned int width, unsigned int height,
                void* data, unsigned int datax, unsigned int datay, unsigned int stride)
{
    unsigned int row, col;
    if (!width || !height) return;
    displaylcd_setup(x, x + width - 1, y, y + height - 1, false);
    for (row = 0; row < height; row++)
    {
        const uint8_t* src = (const uint8_t*)data + ((datay + row) * stride + datax) * 3;
        for (col = 0; col < width; col++, src += 3)
        {
            /* emCORE 24-bit framebuffers are R,G,B bytes in memory (the 4G dither asm sends p[0] as the high byte) */
            uint16_t pixel = ((src[0] >> 3) << 11) | ((src[1] >> 2) << 5) | (src[2] >> 3);
            if (!lcd_wait_tx()) break;
            LCDWDATA = pixel;
        }
    }
    mutex_unlock(&lcd_mutex);
}

void filllcd(unsigned int x, unsigned int y, unsigned int width, unsigned int height, int color)
{
    if (!width || !height) return;
    filllcd_native(x, x + width - 1, y, y + height - 1, color);
}

void INT_DMAC0C4()
{
    DMAC0INTTCCLR = 0x10;
    DMAC0INTERRCLR = 0x10;      /*  an error left pending would re-enter this IRQ for ever */
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


/* Module lease (module5g.h, ABI 2): the controller, DMAC0 channel 4 and the panel belong to the leaseholder.
   lcd_leased is set before the mutex is taken, so console output (cprintf from any thread, the DMA completion
   callback, panic output) only marks the framebuffer dirty instead of touching the controller or blocking. */
static volatile bool lcd_leased;

bool lcd_console_suspended(void)
{
    return lcd_leased;
}

int lcd_lease_acquire(int timeout)
{
    long start;
    lcd_leased = true;
    if (mutex_lock(&lcd_mutex, timeout) != THREAD_OK)
    {
        lcd_leased = false;
        return -1;
    }
    /* let the frame in flight finish (DMA timeout guard), then stop the channel if it did not */
    start = USEC_TIMER;
    while (lcd_dma_busy && !TIMEOUT_EXPIRED(start, LCD_DMA_TIMEOUT_US)) sleep(1000);
    if (lcd_dma_busy || (DMAC0C4CONFIG & 1)) lcd_dma_abort();
    return 0;
}

/* Back to the lcd_panel_init state: gates, CLKCON2, controller modes (steps 1..3), channel 4 stopped with its flags
   cleared (the lcd_dma_abort writes), the detected panel's initialization routine without the white fill, then a full repaint
   of the console framebuffer. A panel that was never detected is left uninitialized, as at boot. */
void lcd_lease_release(void)
{
    long start;
    clockgate_enable(CLOCKGATE_LCD, true);
    clockgate_enable(CLOCKGATE_LCD_2, true);
    clockgate_enable(CLOCKGATE_DMAC0, true);
    CLKCON2 = LCD_CLKCON2_VALUE;
    start = USEC_TIMER;
    while (CLKCON2 != LCD_CLKCON2_VALUE) if (TIMEOUT_EXPIRED(start, 100000)) break;
    DMAC0C4CONFIG = 0;
    DMAC0INTTCCLR = 0x10;
    DMAC0INTERRCLR = 0x10;
    lcd_dma_busy = false;
    clockgate_dma(0, 4, false);
    DMAC0CONFIG = 1;
    LCDCON = LCD_MODE_PIXEL;
    LCDPHTIME = 0x11;
    if (lcd_panel_info.type >= 0) lcd_panels[(int)lcd_panel_info.type].init(false);
    lcd_set_mode(LCD_MODE_PIXEL);
    lcd_leased = false;
    mutex_unlock(&lcd_mutex);
    lcdconsole_update();
}

/* After release: wait for the repaint (bounded by the DMA timeout guard) and report the driver's own counters. */
int lcd_lease_verify(uint32_t* out, uint32_t words)
{
    uint32_t v[N5G_LEASE_VERIFY_WORDS], i;
    displaylcd_sync();
    v[0] = (uint32_t)(int)lcd_panel_info.type;
    v[1] = lcd_panel_info.status_timeouts;
    v[2] = lcd_panel_info.read_timeouts;
    v[3] = lcd_dma_aborts;
    v[4] = LCDCON;
    v[5] = LCDPHTIME;
    v[6] = CLKCON2;
    v[7] = lcd_dma_busy;
    v[8] = DMAC0C4CONFIG;
    v[9] = lcd_leased;
    v[10] = v[11] = 0;
    for (i = 0; i < words && i < N5G_LEASE_VERIFY_WORDS; i++) out[i] = v[i];
    return 0;
}
