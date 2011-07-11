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
#include "timer.h"
#include "power.h"


long power_last_update;
bool power_last_state;


void reset();


void power_off(void)
{
    reset();
}

void power_init(void)
{
    power_last_update = 0;
}

bool charging_state(void)
{
    return false;
}

bool external_power_state(void)
{
    return vbus_state();
}

bool vbus_state(void)
{
    if (TIMEOUT_EXPIRED(power_last_update, 200000))
    {
        power_last_update = USEC_TIMER;
        power_last_state = !!(i2c_recvbyte(0, 0xe6, 4) & 0x40);
    }
    return power_last_state;
}
