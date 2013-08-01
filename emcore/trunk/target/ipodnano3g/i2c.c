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
#include "thread.h"
#include "s5l8702.h"
#include "clockgates.h"


static struct mutex i2cmutex;


static void i2c_on(int bus)
{
    /* enable I2C clock */
    clockgate_enable(CLOCKGATE_I2C(bus), true);

    IICCON(bus) = (1 << 7) | /* ACK_GEN */
                  (0 << 6) | /* CLKSEL = PCLK/16 */
                  (1 << 5) | /* INT_EN */
                  (1 << 4) | /* IRQ clear */
                  (7 << 0);  /* CK_REG */

    /* serial output on */
    IICSTAT(bus) = (1 << 4);
}

static void i2c_off(int bus)
{
    /* serial output off */
    IICSTAT(bus) = 0;

    /* disable I2C clock */
    clockgate_enable(CLOCKGATE_I2C(bus), false);
}

void i2c_init()
{
    mutex_init(&i2cmutex);
}

void i2c_send(uint32_t bus, uint32_t device, uint32_t address, const uint8_t* data, uint32_t length)
{
    mutex_lock(&i2cmutex, TIMEOUT_BLOCK);
	i2c_on(bus);
    while (IIC10(bus));
    IICDS(bus) = device & ~1;
    while (IIC10(bus));
    IICSTAT(bus) = 0xF0;
    while (IIC10(bus));
    IICCON(bus) = 0xB7;
    while (IIC10(bus));
    while ((IICCON(bus) & 0x10) == 0) yield();
    if (address >= 0)
    {
        /* write address */
        while (IIC10(bus));
        IICDS(bus) = address;
        while (IIC10(bus));
        IICCON(bus) = 0xB7;
        while (IIC10(bus));
        while ((IICCON(bus) & 0x10) == 0) yield();
    }
    /* write data */
    while (length--)
    {
        while (IIC10(bus));
        IICDS(bus) = *data++;
        while (IIC10(bus));
        IICCON(bus) = 0xB7;
        while (IIC10(bus));
        while ((IICCON(bus) & 0x10) == 0) yield();
    }
    /* STOP */
    while (IIC10(bus));
    IICSTAT(bus) = 0xD0;
    while (IIC10(bus));
    IICCON(bus) = 0xB7;
    while ((IICSTAT(bus) & (1 << 5)) != 0) yield();
	i2c_off(bus);
    mutex_unlock(&i2cmutex);
}

void i2c_recv(uint32_t bus, uint32_t device, uint32_t address, uint8_t* data, uint32_t length)
{
    mutex_lock(&i2cmutex, TIMEOUT_BLOCK);
	i2c_on(bus);
    if (address >= 0)
    {
        /* START */
        while (IIC10(bus));
        IICDS(bus) = device & ~1;
        while (IIC10(bus));
        IICSTAT(bus) = 0xF0;
        while (IIC10(bus));
        IICCON(bus) = 0xB7;
        while (IIC10(bus));
        while ((IICCON(bus) & 0x10) == 0) yield();
        /* write address */
        while (IIC10(bus));
        IICDS(bus) = address;
        while (IIC10(bus));
        IICCON(bus) = 0xB7;
        while (IIC10(bus));
        while ((IICCON(bus) & 0x10) == 0) yield();
    }
    /* (repeated) START */
    while (IIC10(bus));
    IICDS(bus) = device | 1;
    while (IIC10(bus));
    IICSTAT(bus) = 0xB0;
    while (IIC10(bus));
    IICCON(bus) = 0xB7;
    while (IIC10(bus));
    while ((IICCON(bus) & 0x10) == 0) yield();
    while (length--)
    {
        while (IIC10(bus));
        IICCON(bus) = (length == 0) ? 0x37 : 0xB7; /* NACK or ACK */
        while (IIC10(bus));
        while ((IICCON(bus) & 0x10) == 0) yield();
        *data++ = IICDS(bus);
    }
    /* STOP */
    while (IIC10(bus));
    IICSTAT(bus) = 0x90;
    while (IIC10(bus));
    IICCON(bus) = 0xB7;
    while (IIC10(bus));
    while ((IICSTAT(bus) & (1 << 5)) != 0) yield();
	i2c_off(bus);
    mutex_unlock(&i2cmutex);
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
