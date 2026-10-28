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

//    iPod nano 5G timers.
//    timer_init = bl_PreEfi timers_init 0x2200b8bc: CMD = 0x10 for timers 0..3 and 5..7, TSTAT47 = -1; timer 4 is the
//    free-running microsecond counter (CON 0x40, PRE 0xb, DATA0 -1, CMD 3) started by stage1.S and left alone here.
//    Scheduler wake-up = timer 1 ("B", 0x3c700020, IRQ 8 via TCON status bits 16..18) programmed like Apple's
//    bootloader tick (bl_Timer:0x2fc: CON 0x40 | int-enable 0x1000, PRE 0xb, DATA0 = period, CMD 3), i.e. the same
//    oscillator-derived 1 us tick as timer 4; no PCLK assumption (the 4G's "PCLK/64, usecs*133>>6" has no 5G counterpart).
//    PROVISIONAL: CON b6 / PRE 0xb = 1 MHz is inferred from PreEfi's udelay using the same setup for timer 4
//    16-bit DATA0 -> at most 65535 us per shot (the scheduler re-arms).

#include "global.h"
#include "timer.h"
#include "thread.h"
#include "s5l8730.h"


void timer_init()
{
    TACMD = 0x10;   /* timers 0..3, 5..7: bl_PreEfi:0x2200b8bc writes CMD = 0x10 (stop/clear, inf) */
    TBCMD = 0x10;
    TCCMD = 0x10;
    TDCMD = 0x10;
    TFCMD = 0x10;
    TGCMD = 0x10;
    THCMD = 0x10;
    TSTAT47 = 0xffffffff;   /* 0x3c700118 = -1: clear the timer 4..7 status word */
    /* timer 4 (TE*, 0x3c7000a0) keeps running: it is USEC_TIMER */
}

void timer_schedule_wakeup(uint32_t usecs)
{
    if (usecs > 65535) usecs = 65535;
    if (!usecs) usecs = 1;
    TBCMD = 0x10;
    TBPRE = 0xb;                /* bl_Timer:0x2fc / bl_PreEfi:0x2200b8bc: prescaler 0xb with CON b6 -> 1 us ticks */
    TBDATA0 = usecs;
    TBCON = 0x1040;             /* b12 = interrupt enable (TCON status in bits 16..18, enables 12..14), b6 = clock select */
    TBCMD = 3;                  /* bl_Timer: CMD 3 = enable + start */
}

void timer_kill_wakeup()
{
    TBCMD = 0x10;
    TBCON = TBCON;              /* w1c of the status bits 16..18 */
}

void INT_TIMERB(void)
{
    TBCMD = 0x10;               /* one shot: stop it (CON 0x40 has no one-shot mode bit we know of) */
    TBCON = TBCON;
    scheduler_switch(NULL, NULL);
}
