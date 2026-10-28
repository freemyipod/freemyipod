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

//    iPod nano 5G I2C (same controller as the 4G at 0x3c600000 / 0x3c900000). The transfers are the
//    4G's polled 0xB7 scheme. Difference to the 4G: nothing initialises the controller before emCORE on the DFU path
//    (the ROM does not touch it, the stage-1 loader does not either), so i2c_init() replays PreEfi's i2c0_init
//    (bl_PreEfi:0x2200b400 = fw_disk:0x08071c84) on bus 0 (PMU) and bus 1, after opening the
//    clock gates (IIC0 = PWRCON1 b4 + PWRCON4 b11, IIC1 = PWRCON1 b6 + PWRCON4 b12).
//    All waits are bounded (harness rule): a stuck bus reports through i2c_timeouts instead of hanging.

#include "global.h"
#include "i2c.h"
#include "thread.h"
#include "timer.h"
#include "s5l8730.h"
#include "clockgates.h"


static struct mutex i2cmutex;
uint32_t i2c_timeouts;                 /* host-readable: number of bounded waits that expired */

#define I2C_WAIT_US 100000             /* fw_disk transfer timeouts are 100000 us  */

static void i2c_wait_pending(uint32_t bus)
{
    long start = USEC_TIMER;
    while ((IICCON(bus) & 0x10) == 0)
    {
        if (TIMEOUT_EXPIRED(start, I2C_WAIT_US)) { i2c_timeouts++; return; }
        yield();
    }
}

static void i2c_wait_stop(uint32_t bus)
{
    long start = USEC_TIMER;
    while ((IICSTAT(bus) & (1 << 5)) != 0)
    {
        if (TIMEOUT_EXPIRED(start, I2C_WAIT_US)) { i2c_timeouts++; return; }
        yield();
    }
}

/* bl_PreEfi:0x2200b400: waits "+0x10 == 0" before each write, then
   ADD = 0x40, +0x14 = 1, +0x18 = 0, STAT = 0x80, CON = 0, STAT = 0, DS = 0x40, CON = 0x181, STAT = 0x10, +0x28 = 0 */
static void i2c_wait_unk10(uint32_t bus)
{
    long start = USEC_TIMER;
    while (IICUNK10(bus) != 0) if (TIMEOUT_EXPIRED(start, 10000)) { i2c_timeouts++; return; }
}

static void i2c_controller_init(uint32_t bus)
{
    i2c_wait_unk10(bus); IICADD(bus) = 0x40;
    i2c_wait_unk10(bus); IICUNK14(bus) = 1;
    i2c_wait_unk10(bus); IICUNK18(bus) = 0;
    i2c_wait_unk10(bus); IICSTAT(bus) = 0x80;
    i2c_wait_unk10(bus); IICCON(bus) = 0;
    i2c_wait_unk10(bus); IICSTAT(bus) = 0;
    i2c_wait_unk10(bus); IICDS(bus) = 0x40;
    i2c_wait_unk10(bus); IICCON(bus) = 0x181;
    i2c_wait_unk10(bus); IICSTAT(bus) = 0x10;
    i2c_wait_unk10(bus); IICUNK28(bus) = 0;
}

void i2c_init()
{
    mutex_init(&i2cmutex);
    clockgate_enable(CLOCKGATE_I2C0, true);
    clockgate_enable(CLOCKGATE_I2C0_2, true);
    clockgate_enable(CLOCKGATE_I2C1, true);
    clockgate_enable(CLOCKGATE_I2C1_2, true);
    i2c_controller_init(0);
    i2c_controller_init(1);
}

void i2c_send(uint32_t bus, uint32_t device, uint32_t address, const uint8_t* data, uint32_t length)
{
    mutex_lock(&i2cmutex, TIMEOUT_BLOCK);
    IICDS(bus) = device & ~1;
    IICSTAT(bus) = 0xF0;
    IICCON(bus) = 0xB7;
    i2c_wait_pending(bus);
    if (address >= 0)
    {
        /* write address */
        IICDS(bus) = address;
        IICCON(bus) = 0xB7;
        i2c_wait_pending(bus);
    }
    /* write data */
    while (length--)
    {
        IICDS(bus) = *data++;
        IICCON(bus) = 0xB7;
        i2c_wait_pending(bus);
    }
    /* STOP */
    IICSTAT(bus) = 0xD0;
    IICCON(bus) = 0xB7;
    i2c_wait_stop(bus);
    mutex_unlock(&i2cmutex);
}

void i2c_recv(uint32_t bus, uint32_t device, uint32_t address, uint8_t* data, uint32_t length)
{
    mutex_lock(&i2cmutex, TIMEOUT_BLOCK);
    if (address >= 0)
    {
        /* START */
        IICDS(bus) = device & ~1;
        IICSTAT(bus) = 0xF0;
        IICCON(bus) = 0xB7;
        i2c_wait_pending(bus);
        /* write address */
        IICDS(bus) = address;
        IICCON(bus) = 0xB7;
        i2c_wait_pending(bus);
    }
    /* (repeated) START */
    IICDS(bus) = device | 1;
    IICSTAT(bus) = 0xB0;
    IICCON(bus) = 0xB7;
    i2c_wait_pending(bus);
    while (length--)
    {
        IICCON(bus) = (length == 0) ? 0x37 : 0xB7; /* NACK or ACK */
        i2c_wait_pending(bus);
        *data++ = IICDS(bus);
    }
    /* STOP */
    IICSTAT(bus) = 0x90;
    IICCON(bus) = 0xB7;
    i2c_wait_stop(bus);
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
    buf[0] = 0xff;
    i2c_recv(bus, device, address, buf, 1);
    return buf[0];
}

