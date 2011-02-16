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
#include "s5l8702.h"
#include "power.h"
#include "pmu.h"


void reset();


void power_off(void)
{
    pmu_set_wake_condition(0x42); /* USB inserted or EXTON1 */
    pmu_enter_standby();

    reset();
}

void power_init(void)
{
    pmu_init();
}                          

bool charging_state(void)
{
    return false;
}

bool external_power_state(void)
{
    return (PDAT(12) & 8) ? false : true;
}

bool vbus_state(void)
{
    return (PDAT(12) & 8) ? false : true;
}

int read_battery_voltage(int battery)
{
    if (battery == 0) return pmu_read_battery_voltage();
    return -1;
}

int read_battery_current(int battery)
{
    if (battery == 0) return pmu_read_battery_current();
    return -1;
}

int read_battery_mwh_design(int battery)
{
    if (battery == 0) return 2035;
    return -1;
}

int read_battery_mwh_full(int battery)
{
    if (battery == 0) return 2035;
    return -1;
}

int read_battery_mwh_current(int battery)
{
    // TODO: Approximate that better
    if (battery == 0) return (read_battery_voltage(0) - 3600) * 3;
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
    if (charging_state()) return BATTERY_STATE_CHARGING;
    if (external_power_state()) return BATTERY_STATE_IDLE;
    return BATTERY_STATE_DISCHARGING;
}
