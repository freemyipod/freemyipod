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
#include "s5l8702.h"
#include "power.h"
#include "pmu.h"


void reset();


void power_off(void)
{
    pmu_ldo_on_in_standby(0, 0);
    pmu_ldo_on_in_standby(1, 0);
    pmu_ldo_on_in_standby(2, 0);
    pmu_ldo_on_in_standby(3, 0);
    pmu_ldo_on_in_standby(4, 0);
    pmu_ldo_on_in_standby(5, 0);
    pmu_ldo_on_in_standby(6, 0);
    pmu_ldo_on_in_standby(7, 0);
    pmu_set_wake_condition(0x42); /* USB inserted or EXTON1 */
    pmu_enter_standby();

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
    return (PDAT(12) & 8) ? false : true;
}

bool vbus_state(void)
{
    return (PDAT(12) & 8) ? false : true;
}
