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
#include "power.h"


void reset();


void power_off(void)
{
    reset();
}

void power_init(void)
{
}

bool charging_state(void)
{
    return false;
}

bool external_power_state(void)
{
    return false;
}

bool vbus_state(void)
{
    return true;
}

int read_battery_voltage(int battery)
{
    return -1;
}

int read_battery_current(int battery)
{
    return -1;
}

int read_battery_mwh_design(int battery)
{
    return -1;
}

int read_battery_mwh_full(int battery)
{
    return -1;
}

int read_battery_mwh_current(int battery)
{
    return -1;
}

int read_battery_mw(int battery)
{
    return -1;
}

int read_input_mw(int battery)
{
    return -1;
}

enum battery_state read_battery_state(int battery)
{
    if (battery != 0) return BATTERY_STATE_INVALID;
    return BATTERY_STATE_UNKNOWN;
}
