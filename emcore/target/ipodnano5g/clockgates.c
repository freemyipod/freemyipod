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

//    iPod nano 5G clock gates: unchanged from the 4G (PWRCON0..4 = 0x3c500048/4c/58/68/6c, bit set =
//    clock off; fw_disk:0x0802c5b4). Gate numbers = word index * 32 + bit (s5l8730.h). Blocks with a second bit in
//    PWRCON4 (I2C, LCD, timers, SPI, UART, SWI) enable both numbers.

#include "clockgates.h"
#include "s5l8730.h"
#include "contextswitch.h"


uint32_t clockgates_dma[2];

bool clockgate_get_state(int gate)
{
    return !(PWRCON(gate >> 5) & (1 << (gate & 0x1f)));
}

void clockgate_enable(int gate, bool enable)
{
    uint32_t mode = enter_critical_section();
    if (enable) PWRCON(gate >> 5) &= ~(1 << (gate & 0x1f));
    else PWRCON(gate >> 5) |= 1 << (gate & 0x1f);
    leave_critical_section(mode);
}

/* DMAC0/DMAC1 gates are PWRCON0 b11/b12 (Apple ids 5/4); stage 1 opens them (PWRCON0 = 0x001fa7e5
   leaves b11/b12 clear) and they are never closed: the LCD DMA runs on channel 4 all the time, as on the 4G. */
void clockgate_dma(int dmac, int chan, bool enable)
{
    uint32_t mode = enter_critical_section();
    if (enable) clockgates_dma[dmac] |= 1 << chan;
    else clockgates_dma[dmac] &= ~(1 << chan);
    leave_critical_section(mode);
}
