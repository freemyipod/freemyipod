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


#ifndef __PMU_H__
#define __PMU_H__


#include "global.h"

unsigned char pmu_read(int address);
void pmu_write(int address, unsigned char val);
void pmu_read_multiple(int address, int count, unsigned char* buffer);
void pmu_write_multiple(int address, int count, unsigned char* buffer);
int pmu_read_adc(unsigned int adc);
int pmu_read_battery_voltage(void);
int pmu_read_battery_current(void);
void pmu_init(void) INITCODE_ATTR;
void pmu_ldo_on_in_standby(unsigned int ldo, int onoff);
void pmu_ldo_set_voltage(unsigned int ldo, unsigned char voltage);
void pmu_ldo_power_on(unsigned int ldo);
void pmu_ldo_power_off(unsigned int ldo);
void pmu_set_wake_condition(unsigned char condition);
void pmu_enter_standby(void);
void pmu_read_rtc(unsigned char* buffer);
void pmu_write_rtc(unsigned char* buffer);


#endif
