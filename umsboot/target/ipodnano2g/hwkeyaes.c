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
#include "s5l8701.h"
#include "thread.h"


void hwkeyaes(enum hwkeyaes_direction direction, uint32_t keyidx, void* data, uint32_t size)
{
    uint32_t ptr, i;
    uint32_t go = 1;
    PWRCON(1) &= ~0x400;
    AESTYPE = keyidx;
    AESUNKREG0 = 1;
    AESUNKREG0 = 0;
    AESCONTROL = 1;
    AESKEYLEN = direction == HWKEYAES_ENCRYPT ? 9 : 8;
    AESOUTSIZE = size;
    AESAUXSIZE = 0x10;
    AESINSIZE = 0x10;
    AESSIZE3 = 0x10;
    ptr = direction == HWKEYAES_ENCRYPT ? 0 : (size >> 2) - 4;
    while (true)
    {
        if (direction == HWKEYAES_ENCRYPT)
        {
            if (ptr >= (size >> 2)) break;
            if (ptr != 0)
                for (i = 0; i < 4; i++)
                    ((uint32_t*)data)[ptr + i] ^= ((uint32_t*)data)[ptr + i - 4];
        }
        AESOUTADDR = (uint32_t)data + (ptr << 2);
        AESINADDR = (uint32_t)data + (ptr << 2);
        AESAUXADDR = (uint32_t)data + (ptr << 2);
	    clean_dcache();
        AESSTATUS = 6;
        AESGO = go;
        go = 3;
        while ((AESSTATUS & 6) == 0) yield();
	    invalidate_dcache();
        if (direction == HWKEYAES_DECRYPT)
		{
			if (!ptr) break;
            for (i = 0; i < 4; i++)
                ((uint32_t*)data)[ptr + i] ^= ((uint32_t*)data)[ptr + i - 4];
		}
        ptr += direction == HWKEYAES_ENCRYPT ? 4 : -4;
    }
    AESCONTROL = 0;
    PWRCON(1) |= 0x400;
}
