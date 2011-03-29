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
#include "i2c.h"
#include "s5l8702.h"


void i2c_init()
{
}

void i2c_send(uint32_t bus, uint32_t device, uint32_t address, const uint8_t* data, uint32_t length)
{
    IICDS(bus) = device & ~1;
    IICSTAT(bus) = 0xF0;
    IICCON(bus) = 0xB7;
    asm volatile("nop\n\t");
    while ((IICCON(bus) & 0x10) == 0);
    if (address >= 0)
    {
        /* write address */
        IICDS(bus) = address;
        IICCON(bus) = 0xB7;
        asm volatile("nop\n\t");
        while ((IICCON(bus) & 0x10) == 0);
    }
    /* write data */
    while (length--)
    {
        IICDS(bus) = *data++;
        IICCON(bus) = 0xB7;
        asm volatile("nop\n\t");
        while ((IICCON(bus) & 0x10) == 0);
    }
    /* STOP */
    IICSTAT(bus) = 0xD0;
    IICCON(bus) = 0xB7;
    asm volatile("nop\n\t");
    while ((IICSTAT(bus) & (1 << 5)) != 0);
}

void i2c_recv(uint32_t bus, uint32_t device, uint32_t address, uint8_t* data, uint32_t length)
{
    if (address >= 0)
    {
        /* START */
        IICDS(bus) = device & ~1;
        IICSTAT(bus) = 0xF0;
        IICCON(bus) = 0xB7;
        while ((IICCON(bus) & 0x10) == 0);
        /* write address */
        IICDS(bus) = address;
        IICCON(bus) = 0xB7;
        while ((IICCON(bus) & 0x10) == 0);
    }
    /* (repeated) START */
    IICDS(bus) = device | 1;
    IICSTAT(bus) = 0xB0;
    IICCON(bus) = 0xB7;
    while ((IICCON(bus) & 0x10) == 0);
    while (length--)
    {
        IICCON(bus) = (length == 0) ? 0x37 : 0xB7; /* NACK or ACK */
        while ((IICCON(bus) & 0x10) == 0);
        *data++ = IICDS(bus);
    }
    /* STOP */
    IICSTAT(bus) = 0x90;
    IICCON(bus) = 0xB7;
    while ((IICSTAT(bus) & (1 << 5)) != 0);
}

void i2c_sendbyte(uint32_t bus, uint32_t device, uint32_t address, uint32_t data)
{
    uint8_t buf[1];
    buf[0] = data;
    i2c_send(bus, device, address, buf, 1);
}

uint8_t i2c_recvbyte(uint32_t bus, uint32_t device, uint32_t address)
{
    uint8_t buf[1];
    i2c_recv(bus, device, address, buf, 1);
    return buf[0];
}
