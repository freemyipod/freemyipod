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
#include "hwkeyaes.h"
#include "s5l8702.h"
#include "thread.h"


void hwkeyaes(enum hwkeyaes_direction direction, uint32_t keyidx, void* data, uint32_t size)
{
    int i;
    clockgate_enable(7, true);
    for (i = 0; i < 4; i++) AESIV[i] = 0;
    AESUNKREG0 = 1;
    AESUNKREG0 = 0;
    AESCONTROL = 1;
    AESUNKREG1 = 0;
    AESTYPE = keyidx;
    AESTYPE2 = ~AESTYPE;
    AESUNKREG2 = 0;
    AESKEYLEN = direction == HWKEYAES_ENCRYPT ? 9 : 8;
    AESOUTSIZE = size;
    AESOUTADDR = data;
    AESINSIZE = size;
    AESINADDR = data;
    AESAUXSIZE = size;
    AESAUXADDR = data;
    AESSIZE3 = size;
	clean_dcache();
    AESGO = 1;
	invalidate_dcache();
    while (!(AESSTATUS & 0xf)) sleep(100);
    clockgate_enable(7, false);
}
