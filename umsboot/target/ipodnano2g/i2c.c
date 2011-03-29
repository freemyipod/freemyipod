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
#include "s5l8701.h"


void i2c_init()
{
    /* enable I2C pins */
    PCON10 = (2 << 2) |
             (2 << 0);

    /* enable I2C clock */
    PWRCON(0) &= ~(1 << 5);

    /* initial config */
    IICADD = 0;
    IICCON = (1 << 7) | /* ACK_GEN */
             (0 << 6) | /* CLKSEL = PCLK/16 */
             (1 << 5) | /* INT_EN */
             (1 << 4) | /* IRQ clear */
             (7 << 0);  /* CK_REG */

    /* serial output on */
    IICSTAT = (1 << 4);
}

void i2c_send(uint32_t bus, uint32_t device, uint32_t address, const uint8_t* data, uint32_t length)
{
    IICDS = device & ~1;
    IICSTAT = 0xF0;
    IICCON = 0xB7;
        while ((IICCON & 0x10) == 0);
    if (address >= 0)
    {
        /* write address */
        IICDS = address;
        IICCON = 0xB7;
        while ((IICCON & 0x10) == 0);
    }
    /* write data */
    while (length--)
    {
        IICDS = *data++;
        IICCON = 0xB7;
        while ((IICCON & 0x10) == 0);
    }
    /* STOP */
    IICSTAT = 0xD0;
    IICCON = 0xB7;
    while ((IICSTAT & (1 << 5)) != 0);
}

void i2c_recv(uint32_t bus, uint32_t device, uint32_t address, uint8_t* data, uint32_t length)
{
    if (address >= 0)
    {
        /* START */
        IICDS = device & ~1;
        IICSTAT = 0xF0;
        IICCON = 0xB7;
        while ((IICCON & 0x10) == 0);
        /* write address */
        IICDS = address;
        IICCON = 0xB7;
        while ((IICCON & 0x10) == 0);
    }
    /* (repeated) START */
    IICDS = device | 1;
    IICSTAT = 0xB0;
    IICCON = 0xB7;
    while ((IICCON & 0x10) == 0);
    while (length--)
    {
        IICCON = (length == 0) ? 0x37 : 0xB7; /* NACK or ACK */
        while ((IICCON & 0x10) == 0);
        *data++ = IICDS;
    }
    /* STOP */
    IICSTAT = 0x90;
    IICCON = 0xB7;
    while ((IICSTAT & (1 << 5)) != 0);
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
