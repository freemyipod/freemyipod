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
#include "i2c.h"
#include "thread.h"
#include "s5l8720.h"


static struct mutex i2cmutex;
static struct wakeup i2cwakeup;


void i2c_init()
{
    mutex_init(&i2cmutex);
    wakeup_init(&i2cwakeup);

    interrupt_enable(IRQ_IIC, true);
}

void i2c_send(uint32_t bus, uint32_t device, uint32_t address, const uint8_t* data, uint32_t length)
{
    mutex_lock(&i2cmutex, TIMEOUT_BLOCK);

    mutex_unlock(&i2cmutex);
}

void i2c_recv(uint32_t bus, uint32_t device, uint32_t address, uint8_t* data, uint32_t length)
{
    mutex_lock(&i2cmutex, TIMEOUT_BLOCK);

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

void INT_IIC()
{

    wakeup_signal(&i2cwakeup);
}
