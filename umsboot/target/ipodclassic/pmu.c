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
#include "pmu.h"


void pmu_read_multiple(int address, int count, unsigned char* buffer)
{
    i2c_recv(0, 0xe6, address, buffer, count);
}

void pmu_write_multiple(int address, int count, unsigned char* buffer)
{
    i2c_send(0, 0xe6, address, buffer, count);
}

unsigned char pmu_read(int address)
{
    return i2c_recvbyte(0, 0xe6, address);
}

void pmu_write(int address, unsigned char val)
{
    i2c_sendbyte(0, 0xe6, address, val);
}

int pmu_read_adc(unsigned int adc)
{
    int data = 0;
    pmu_write(0x54, 5 | (adc << 4));
    while ((data & 0x80) == 0) data = pmu_read(0x57);
    int value = (pmu_read(0x55) << 2) | (data & 3);
    return value;
}

/* millivolts */
int pmu_read_battery_voltage(void)
{
    return pmu_read_adc(0) * 6;
}

/* milliamps */
int pmu_read_battery_current(void)
{
    return pmu_read_adc(2);
}

void pmu_ldo_on_in_standby(unsigned int ldo, int onoff)
{
    if (ldo < 4)
    {
        unsigned char newval = pmu_read(0x3B) & ~(1 << (2 * ldo));
        if (onoff) newval |= 1 << (2 * ldo);
        pmu_write(0x3B, newval);
    }
    else if (ldo < 8)
    {
        unsigned char newval = pmu_read(0x3C) & ~(1 << (2 * (ldo - 4)));
        if (onoff) newval |= 1 << (2 * (ldo - 4));
        pmu_write(0x3C, newval);
    }
}

void pmu_ldo_set_voltage(unsigned int ldo, unsigned char voltage)
{
    if (ldo > 6) return;
    pmu_write(0x2d + (ldo << 1), voltage);
}

void pmu_ldo_power_on(unsigned int ldo)
{
    if (ldo > 6) return;
    pmu_write(0x2e + (ldo << 1), 1);
}

void pmu_ldo_power_off(unsigned int ldo)
{
    if (ldo > 6) return;
    pmu_write(0x2e + (ldo << 1), 0);
}

void pmu_set_wake_condition(unsigned char condition)
{
    pmu_write(0xd, condition);
}

void pmu_enter_standby(void)
{
    pmu_write(0xc, 1);
}

void pmu_read_rtc(unsigned char* buffer)
{
    pmu_read_multiple(0x59, 7, buffer);
}

void pmu_write_rtc(unsigned char* buffer)
{
    pmu_write_multiple(0x59, 7, buffer);
}
