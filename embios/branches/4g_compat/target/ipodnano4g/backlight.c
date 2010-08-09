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

void backlight_set(uint8_t brightness, uint8_t fade)
{
    i2csendbyte(0xe6, 0x30, brightness);
    i2csendbyte(0xe6, 0x31, 3);
}

void backlight_on(uint8_t fade)
{
    backlight_set(255, 32);
}

void backlight_off(uint8_t fade)
{
    i2csendbyte(0xe6, 0x31, 2);
}
