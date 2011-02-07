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
#include "spi.h"
#include "s5l8702.h"
#include "thread.h"
#include "clockgates.h"
#include "clockgates-target.h"


static struct mutex spimutex[3];
static struct wakeup spiwakeup[3];


void spi_prepare(int port)
{
    mutex_lock(&spimutex[port], TIMEOUT_BLOCK);
    clockgate_enable(SPICLKGATE(port), true);
    SPISTATUS(port) = 0xf;
    SPICTRL(port) |= 0xc;
    SPICLKDIV(port) = 4;
    SPIUNKREG1(port) = 6;
    SPISETUP(port) = 0x10618;
    SPICTRL(port) |= 0xc;
    SPICTRL(port) = 1;
}

void spi_release(int port)
{
    clockgate_enable(SPICLKGATE(port), false);
    mutex_unlock(&spimutex[port]);
}

uint32_t spi_write(int port, uint32_t data)
{
    SPIRXLIMIT(port) = 1;
    while ((SPISTATUS(port) & 0x1f0) == 0x100) yield();
    SPITXDATA(port) = data;
    while (!(SPISTATUS(port) & 0x3e00)) yield();
    return SPIRXDATA(port);
}

void spi_read(int port, uint32_t size, void* buf)
{
    uint8_t* buffer = (uint8_t*)buf;
    SPIRXLIMIT(port) = size;
    if (size < 0x100)
    {
        SPISETUP(port) |= 1;
        while (size--)
        {
            while (!(SPISTATUS(port) & 0x3e00)) yield();
            *buffer++ = SPIRXDATA(port);
        }
        SPISETUP(port) &= ~1;
        return;
    }
    SPISETUP(port) |= 0x41;
    void* addr = (void*)((uint32_t)buf + size);
    struct dma_lli* lli = (struct dma_lli*)(((uint32_t)addr - 0x10) & ~0xf);
    struct dma_lli* nextlli = NULL;
    clockgate_dma(0, port + 5, true);
    while (addr > buf)
    {
        size = (uint32_t)addr - (uint32_t)buf;
        if (size > 0xfff) size = 0xfff;
        else lli = (struct dma_lli*)((int)&DMAC0CLLI(port + 5));
        addr = (void*)((uint32_t)addr - size);
        lli->srcaddr = (void*)((int)&SPIRXDATA(port));
        lli->dstaddr = addr;
        lli->nextlli = nextlli;
        lli->control = 0x78000000 | size | (nextlli == NULL ? 0x80000000 : 0);
        nextlli = lli;
        lli = &lli[-1];
    }
    clean_dcache();
    DMAC0CCONFIG(port + 5) = 0x9001 | (SPIDMA(port) << 1);
    wakeup_wait(&spiwakeup[port], TIMEOUT_BLOCK);
    clockgate_dma(0, port + 5, false);
    invalidate_dcache();
    SPISETUP(port) &= ~0x41;
}

void INT_DMAC0C5()
{
    DMAC0INTTCCLR = 1 << 5;
    wakeup_signal(&spiwakeup[0]);
}

void INT_DMAC0C6()
{
    DMAC0INTTCCLR = 1 << 6;
    wakeup_signal(&spiwakeup[1]);
}

void INT_DMAC0C7()
{
    DMAC0INTTCCLR = 1 << 7;
    wakeup_signal(&spiwakeup[2]);
}
