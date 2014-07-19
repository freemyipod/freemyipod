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


uint32_t clockgates_dma[2];

bool clockgate_get_state(int gate)
{
    return !(PWRCON(gate >> 5) & (1 << (gate & 0x1f)));
}

void clockgate_enable(int gate, bool enable) ICODE_ATTR;
void clockgate_enable(int gate, bool enable)
{
    uint32_t mode = enter_critical_section();
    if (enable) PWRCON(gate >> 5) &= ~(1 << (gate & 0x1f));
    else PWRCON(gate >> 5) |= 1 << (gate & 0x1f);
    leave_critical_section(mode);
}

void clockgate_dma(int dmac, int chan, bool enable)
{
    uint32_t mode = enter_critical_section();
    uint32_t old = clockgates_dma[dmac];
    if (enable) clockgates_dma[dmac] |= 1 << chan;
    else clockgates_dma[dmac] &= ~(1 << chan);
    if (clockgates_dma[dmac] != old)
        clockgate_enable(CLOCKGATE_DMA(dmac), true);
    leave_critical_section(mode);
}
