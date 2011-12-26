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


#ifndef __POWER_H__
#define __POWER_H__


#include "global.h"


enum battery_state
{
    BATTERY_STATE_UNKNOWN,
    BATTERY_STATE_INVALID,
    BATTERY_STATE_NONPRESENT,
    BATTERY_STATE_IDLE,
    BATTERY_STATE_CHARGING,
    BATTERY_STATE_DISCHARGING
};

enum input_state
{
    INPUT_STATE_UNKNOWN,
    INPUT_STATE_INVALID,
    INPUT_STATE_NONPRESENT,
    INPUT_STATE_OUTOFRANGE,
    INPUT_STATE_IDLE,
    INPUT_STATE_ACTIVE
};


void power_off(void);
void power_init(void) INITCODE_ATTR;
bool charging_state(void);
bool external_power_state(void);
bool vbus_state(void);
int read_battery_voltage(int battery);
int read_battery_current(int battery);
int read_battery_mwh_design(int battery);
int read_battery_mwh_full(int battery);
int read_battery_mwh_current(int battery);
int read_battery_mw(int battery);
enum battery_state read_battery_state(int battery);
int read_input_voltage(int input);
int read_input_current(int input);
int read_input_mw(int input);
enum input_state read_input_state(int input);


#endif
