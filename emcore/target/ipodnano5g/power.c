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

//    iPod nano 5G power (PMU on I2C bus 0, 7-bit 0x73 = emCORE 0xe6).
//     - vbus_state: PMU 0x04 b6 if 0x0a b3 is set, else b3 (fw_disk:0x08056670 + 0x0807117c)
//     - power_off: PMU 0x31 = 2 then 0x0a = 1 (fw_disk:0x08056592; PreEfi 0x2200afbc)
//     - USB budget: the stock OS raises the PMU input limit to 500 mA only AFTER SET_CONFIGURATION (fw_disk:0x08050c18
//       -> mode 6 at 0x0805063c -> pmu_set_usb_input_mode 0x08071374: 0x0a &= ~8, 0x33 = (0x33 & ~3) | 2 = 0x52;
//       pmu_enable_charger 0x080714b4: 0x34 = 0x7c) and drops back to 100 mA on set-address/reset (0x08050988). The USB
//       IRQ (usb/usbglue.c) only publishes the budget through n5g_power_usb_budget(); a CORE thread applies it over I2C.
//       Never 1 A (mode 7, Apple charger only), never 0x0a bits 0..3, never before the host configured the device.
//       Every PMU write prints its plan on CONSOLE_BOOT first (nothing survives a reset).

#include "global.h"
#include "i2c.h"
#include "timer.h"
#include "power.h"
#include "console.h"
#include "thread.h"


#define PMU_BUS 0
#define PMU_ADDR 0xe6


long power_last_update;
bool power_last_state;

static volatile int n5g_usb_budget_ma = 100;    /* written by the USB IRQ, read by the power thread */
static volatile int n5g_power_applied_ma = 100; /* what the PMU really holds (read back after each write) */
static struct scheduler_thread n5g_power_thread_handle;
static uint32_t n5g_power_stack[0x200];

static uint8_t pmu_rd(uint32_t reg) { return i2c_recvbyte(PMU_BUS, PMU_ADDR, reg); }
static void pmu_wr(uint32_t reg, uint32_t v) { i2c_sendbyte(PMU_BUS, PMU_ADDR, reg, v & 0xff); }


void power_off(void)
{
    cprintf(CONSOLE_BOOT, "[power] off: PMU 31=02, 0a=01 (fw_disk:0x08056592)\n");
    pmu_wr(0x31, 2);
    pmu_wr(0x0a, 1);
    while (true) sleep(1000000);
}

static void n5g_power_thread(void* a0, void* a1, void* a2, void* a3);

void power_init(void)
{
    uint8_t r2, r4, r0a, r30, r31, r33, r34;
    power_last_update = 0;
    r2 = pmu_rd(0x02); r4 = pmu_rd(0x04); r0a = pmu_rd(0x0a); r30 = pmu_rd(0x30); r31 = pmu_rd(0x31);
    r33 = pmu_rd(0x33); r34 = pmu_rd(0x34);
    cprintf(CONSOLE_BOOT, "[power] pmu 02=%02X 04=%02X 0a=%02X 30=%02X 31=%02X 33=%02X 34=%02X\n",
            r2, r4, r0a, r30, r31, r33, r34);
    /* bl_PreEfi pmu_init 0x2200b650: a set warm flag (reg 2 b7) would make Apple's bootloader take the hibernate
       path on the next boot; clear it like PreEfi does. */
    if (r2 & 0x80)
    {
        cprintf(CONSOLE_BOOT, "[power] plan 02 %02X->%02X (warm flag)\n", r2, r2 & 0x7f);
        pmu_wr(0x02, r2 & 0x7f);
    }
    if (!thread_create(&n5g_power_thread_handle, "power", n5g_power_thread, n5g_power_stack, sizeof(n5g_power_stack),
                       CORE_THREAD, 100, true, NULL, NULL, NULL, NULL))
        cprintf(CONSOLE_BOOT, "[power] no thread\n");
}

bool charging_state(void)
{
    return false;
}

bool external_power_state(void)
{
    return vbus_state();
}

/* input present = PMU 0x04 b6 when 0x0a b3 is set, else 0x04 b3 (fw_disk:0x08056670, 0x0807117c) */
bool vbus_state(void)
{
    if (TIMEOUT_EXPIRED(power_last_update, 200000))
    {
        uint8_t r0a, r4;
        power_last_update = USEC_TIMER;
        r0a = pmu_rd(0x0a);
        r4 = pmu_rd(0x04);
        power_last_state = (r0a & 0x08) ? !!(r4 & 0x40) : !!(r4 & 0x08);
    }
    return power_last_state;
}

/* bl_PreEfi pmu_adc_battery_mv 0x2200c77c: 0x40 = ch | 0x41, poll b4, value = 0x42 << 2 | 0x41 & 3,
   mV = adc * 2000 / 1024 + 2500. This driver does not select an ADC battery channel;
   read_battery_voltage() returns -1. */
int read_battery_voltage(int battery)
{
    return -1;
}

int read_battery_current(int battery)
{
    return -1;
}

int read_battery_mwh_design(int battery)
{
    return -1;
}

int read_battery_mwh_full(int battery)
{
    return -1;
}

int read_battery_mwh_current(int battery)
{
    return -1;
}

int read_battery_mw(int battery)
{
    return -1;
}

enum battery_state read_battery_state(int battery)
{
    if (battery != 0) return BATTERY_STATE_INVALID;
    return BATTERY_STATE_UNKNOWN;
}

int read_input_voltage(int input)
{
    return -1;
}

int read_input_current(int input)
{
    return -1;
}

int read_input_mw(int input)
{
    return -1;
}

enum input_state read_input_state(int input)
{
    if (input != 0) return INPUT_STATE_INVALID;
    return vbus_state() ? INPUT_STATE_ACTIVE : INPUT_STATE_NONPRESENT;
}


/* ---- USB power budget  --------------------------------------------------------------------- */

/* what the applied budget is right now (a later NAND bring-up may gate on it) */
int n5g_power_budget_applied_ma(void)
{
    return n5g_power_applied_ma;
}

/* IRQ context (usb/usbglue.c set/unset_configuration): only a store, no I2C. Name = the shared hook's (4G). */
void n5g_power_usb_budget(int ma)
{
    n5g_usb_budget_ma = ma;
}

/* thread context only: mode 6 of fw_disk 0x08071374 + charger on 0x080714b4 */
static void n5g_usb_500(void)
{
    uint8_t r4 = pmu_rd(0x04), r0a = pmu_rd(0x0a), r33 = pmu_rd(0x33), r34 = pmu_rd(0x34);
    uint8_t v33 = (r33 & 0xfc) | 0x02;   /* [1:0] = 2 = 500 mA -> 0x52 with the boot value 0x50 */
    cprintf(CONSOLE_BOOT, "[power] cfg 04=%02X 0a=%02X 33=%02X 34=%02X\n", r4, r0a, r33, r34);
    if ((r0a & 0x03) || (v33 & 3) != 2)
    {
        cprintf(CONSOLE_BOOT, "[power] precond fail: stay at 100 mA\n");
        return;
    }
    cprintf(CONSOLE_BOOT, "[power] plan 0a=%02X 33=%02X 34=7c\n", r0a & 0xf7, v33);
    if (r0a & 0x08) pmu_wr(0x0a, r0a & 0xf7);   /* 0x08071374 mode 6: clear b3 only (input on) */
    pmu_wr(0x33, v33);
    pmu_wr(0x34, 0x7c);                          /* 0x080714b4 charger on (4G wrote 0x04) */
    r4 = pmu_rd(0x04); r0a = pmu_rd(0x0a); r33 = pmu_rd(0x33); r34 = pmu_rd(0x34);
    cprintf(CONSOLE_BOOT, "[power] USB limit %s mA (04=%02X 0a=%02X 33=%02X 34=%02X)\n",
            (r33 & 3) == 2 ? "500" : "??", r4, r0a, r33, r34);
    if ((r33 & 3) == 2) n5g_power_applied_ma = 500;
}

static void n5g_usb_100(void)
{
    uint8_t r33 = pmu_rd(0x33);
    cprintf(CONSOLE_BOOT, "[power] uncfg plan 33 %02X->%02X\n", r33, r33 & 0xfc);
    pmu_wr(0x33, r33 & 0xfc);                    /* modes 2..5: 0x33 = 0x50 (100 mA class), 0x34 untouched */
    n5g_power_applied_ma = 100;
    cprintf(CONSOLE_BOOT, "[power] USB limit 100 mA (33=%02X)\n", pmu_rd(0x33));
}

static void n5g_power_thread(void* a0, void* a1, void* a2, void* a3)
{
    int applied = 100;
    (void)a0; (void)a1; (void)a2; (void)a3;
    while (true)
    {
        int want;
        sleep(20000);
        want = n5g_usb_budget_ma >= 500 ? 500 : 100;   /* never more than the 500 mA emCORE declares */
        if (want == applied) continue;
        applied = want;
        if (want == 500) n5g_usb_500();
        else n5g_usb_100();
    }
}
