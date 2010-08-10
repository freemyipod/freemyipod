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
#include "console.h"
#include "accel.h"
#include "backlight.h"

void main()
{
  //panic("main() doesn't know what to do!");
  demo();
}

void demo()
{
  backlight_off(32);
  cputs(1, "\nAccelerometer data. Om nom nom!\n\n");
  int i;
  for(i = 0; i < 16; i++)
  {
    uint8_t x = accel_get_axis(0);
    uint8_t y = accel_get_axis(1);
    uint8_t z = accel_get_axis(2);
    cprintf(1, "x:%3d y:%3d z:%3d\n", x, y, z);
  }
  cputs(1, "\nemBIOS is a hardware abstraction with\ndebugging.\n\nIt helps with writing drivers too.");
  backlight_on(32);
  while(1) {}
}
