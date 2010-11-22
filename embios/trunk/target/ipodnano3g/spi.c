//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include "global.h"
#include "spi.h"
#include "s5l8702.h"
#include "thread.h"
#include "clockgates.h"


static struct mutex spimutex[3];


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
    SPISETUP(port) |= 1;
    while (size--)
    {
        while (!(SPISTATUS(port) & 0x3e00)) yield();
        *buffer++ = SPIRXDATA(port);
    }
    SPISETUP(port) &= ~1;
}

