//
//    iPod nano 5G target init hooks. Only targetinit_late is used: I2C and the console are up,
//    USB is started. It reports the state the stage-1 loader left (clocks, SDRAM controller, PMU) on CONSOLE_BOOT, the
//    LCD panel detection result, switches the backlight on at N5G_BACKLIGHT_BOOT (the ROM/DFU path leaves PMU r30/r31 at
//    0, so the console would be unlit) and starts the click wheel poller. Nothing here touches the PLL, the SDRAM
//    controller or the page table.
//
#include "global.h"
#include "targetinit.h"
#include "console.h"
#include "i2c.h"
#include "clickwheel.h"
#include "s5l8730.h"
#include "backlight.h"

/* boot backlight level (target.mk N5G_BACKLIGHT_BOOT; 0 = leave it off), clamped by backlight.c to N5G_BACKLIGHT_MAX */
#ifndef N5G_BACKLIGHT_BOOT
#define N5G_BACKLIGHT_BOOT 0x20
#endif

void lcd_print_panel_info(unsigned int consoles);

#define REG32(a) (*(volatile uint32_t *)(a))

/* host-readable snapshot (symbol n5g_init_report in emcore.map) */
struct n5g_init_report
{
    uint32_t magic;          /* 'N5GI' */
    uint32_t clkcon0, clkcon1, pll0pms, plllock;   /* expect 0x40009000, 0x00404040, 0x0900ce01, b0 */
    uint32_t pwrcon[5];      /* 0x3c500048/4c/58/68/6c */
    uint32_t dram100, dram108, dram140, dram144;   /* 0x3d700100 (|= 0x09100000), 0x108 (0x0002100b), DLL ctrl/status */
    uint32_t sctlr;          /* CP15 c1: M/C/I/XP as crt0 set them */
    uint8_t pmu[8];          /* PMU regs 02 04 05 0a 30 31 33 34 */
    int wheel_rc;
} n5g_init_report;

void targetinit_late(void)
{
    struct n5g_init_report* r = &n5g_init_report;
    r->magic = 0x4947354e;
    r->clkcon0 = CLKCON0; r->clkcon1 = CLKCON1; r->pll0pms = PLL0PMS; r->plllock = PLLLOCK;
    r->pwrcon[0] = PWRCON(0); r->pwrcon[1] = PWRCON(1); r->pwrcon[2] = PWRCON(2); r->pwrcon[3] = PWRCON(3); r->pwrcon[4] = PWRCON(4);
    r->dram100 = REG32(0x3d700100); r->dram108 = REG32(0x3d700108); r->dram140 = REG32(0x3d700140); r->dram144 = REG32(0x3d700144);
    __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(r->sctlr));
    r->pmu[0] = i2c_recvbyte(0, 0xe6, 0x02); r->pmu[1] = i2c_recvbyte(0, 0xe6, 0x04); r->pmu[2] = i2c_recvbyte(0, 0xe6, 0x05);
    r->pmu[3] = i2c_recvbyte(0, 0xe6, 0x0a); r->pmu[4] = i2c_recvbyte(0, 0xe6, 0x30); r->pmu[5] = i2c_recvbyte(0, 0xe6, 0x31);
    r->pmu[6] = i2c_recvbyte(0, 0xe6, 0x33); r->pmu[7] = i2c_recvbyte(0, 0xe6, 0x34);
    cprintf(CONSOLE_BOOT, "[init] clk %08X/%08X pll %08X lock %08X sctlr %08X\n", r->clkcon0, r->clkcon1, r->pll0pms, r->plllock, r->sctlr);
    cprintf(CONSOLE_BOOT, "[init] gates %08X %08X %08X %08X %08X\n", r->pwrcon[0], r->pwrcon[1], r->pwrcon[2], r->pwrcon[3], r->pwrcon[4]);
    cprintf(CONSOLE_BOOT, "[init] dram 100=%08X 108=%08X 140=%08X 144=%08X\n", r->dram100, r->dram108, r->dram140, r->dram144);
    cprintf(CONSOLE_BOOT, "[init] pmu 02=%02X 04=%02X 05=%02X 0a=%02X 30=%02X 31=%02X 33=%02X 34=%02X\n",
            r->pmu[0], r->pmu[1], r->pmu[2], r->pmu[3], r->pmu[4], r->pmu[5], r->pmu[6], r->pmu[7]);
    lcd_print_panel_info(CONSOLE_BOOT);
    if (N5G_BACKLIGHT_BOOT)
    {
        backlight_set_brightness(N5G_BACKLIGHT_BOOT);
        cprintf(CONSOLE_BOOT, "[init] backlight 30=%02X 31=%02X\n", i2c_recvbyte(0, 0xe6, 0x30), i2c_recvbyte(0, 0xe6, 0x31));
    }
#ifdef HAVE_CLICKWHEEL
    r->wheel_rc = clickwheel_init();
    cprintf(CONSOLE_BOOT, "[init] clickwheel rc=%08X\n", r->wheel_rc);
#endif
}
