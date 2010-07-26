//
//
//    Copyright 2009 TheSeven
//
//
//    This file is part of the Linux4Nano toolkit.
//
//    TheSeven's iBugger is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    TheSeven's iBugger is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with the Linux4Nano toolkit.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include <toolkit.h>
#include <hash.h>


void hash(void* data, uint32_t size, void* result)
{
    uint32_t ptr, i;
    uint32_t ctrl = 2;
    PWRCONEXT &= ~4;
    for (ptr = 0; ptr < size; ptr += 0x10)
    {
      for (i = 0; i < 0x10; i++) HASHDATAIN[i] = ((uint32_t*)data)[ptr + i];
      HASHCTRL = ctrl;
      ctrl = 0xA;
      while (HASHCTRL & 1);
    }
    for (i = 0; i < 5; i ++) ((uint32_t*)result)[i] = HASHRESULT[i];
    PWRCONEXT |= 4;
}
