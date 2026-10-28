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

//    iPod nano 5G backlight = PMU registers over I2C:
//    Pmu_SetBacklight bl_DxeD1759:0xd54: v = r0x0a; r0x0a = v & 0x8f; r0x30 = level; r0x31 = (level != 0); r0x0a = v.
//    The 4G's plain 0x30/0x31 writes are kept (same registers); the 0x0a bracket is replayed around the brightness write.
//    N5G_BACKLIGHT_MAX (default 0x40) clamps every level: first boots run from USB on the ROM's 100 mA budget.
//    Stock firmware clamps to 100 on external power.

#include "global.h"
#include "i2c.h"
#include "backlight.h"

#ifndef N5G_BACKLIGHT_MAX
#define N5G_BACKLIGHT_MAX 0x40
#endif

static bool backlight_fade;


void backlight_init()
{
}

void backlight_on(bool on)
{
    i2c_sendbyte(0, 0xe6, 0x31, (backlight_fade ? 2 : 0) + (on ? 1 : 0));
}

void backlight_set_fade(uint8_t fade)
{
    backlight_fade = fade;
}

void backlight_set_brightness(uint8_t brightness)
{
    uint8_t r0a;
    if (brightness > N5G_BACKLIGHT_MAX) brightness = N5G_BACKLIGHT_MAX;
    r0a = i2c_recvbyte(0, 0xe6, 0x0a);
    i2c_sendbyte(0, 0xe6, 0x0a, r0a & 0x8f);       /* bl_DxeD1759:0xd54: clear b4..b6 (SWI backlight mode off) */
    i2c_sendbyte(0, 0xe6, 0x30, brightness);
    i2c_sendbyte(0, 0xe6, 0x31, (backlight_fade ? 2 : 0) + (brightness ? 1 : 0));
    i2c_sendbyte(0, 0xe6, 0x0a, r0a);
}
