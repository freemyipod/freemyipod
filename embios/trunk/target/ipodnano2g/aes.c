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
#include <aes.h>


void aes_encrypt(uint32_t keytype, void* data, uint32_t size)
{
    uint32_t ptr, i;
    uint32_t go = 1;
    PWRCONEXT &= ~0x400;
    AESTYPE = keytype;
    AESUNKREG0 = 1;
    AESUNKREG0 = 0;
    AESCONTROL = 1;
    AESKEYLEN = 9;
    AESOUTSIZE = size;
    AESAUXSIZE = 0x10;
    AESINSIZE = 0x10;
    AESSIZE3 = 0x10;
    for (ptr = 0; ptr < (size >> 2); ptr += 4)
    {
        AESOUTADDR = (uint32_t)data + (ptr << 2);
        AESINADDR = (uint32_t)data + (ptr << 2);
        AESAUXADDR = (uint32_t)data + (ptr << 2);
        if (ptr != 0)
            for (i = 0; i < 4; i++)
                ((uint32_t*)data)[ptr + i] ^= ((uint32_t*)data)[ptr + i - 4];
        clean_dcache();
        AESSTATUS = 6;
        AESGO = go;
        go = 3;
        while ((AESSTATUS & 6) == 0);
        invalidate_dcache();
    }
    AESCONTROL = 0;
    PWRCONEXT |= 0x400;
}

void aes_decrypt(uint32_t keytype, void* data, uint32_t size)
{
    uint32_t ptr, i;
    uint32_t go = 1;
    PWRCONEXT &= ~0x400;
    AESTYPE = keytype;
    AESUNKREG0 = 1;
    AESUNKREG0 = 0;
    AESCONTROL = 1;
    AESKEYLEN = 8;
    AESOUTSIZE = size;
    AESAUXSIZE = 0x10;
    AESINSIZE = 0x10;
    AESSIZE3 = 0x10;
    for (ptr = (size >> 2) - 4; ; ptr -= 4)
    {
        AESOUTADDR = (uint32_t)data + (ptr << 2);
        AESINADDR = (uint32_t)data + (ptr << 2);
        AESAUXADDR = (uint32_t)data + (ptr << 2);
        clean_dcache();
        AESSTATUS = 6;
        AESGO = go;
        go = 3;
        while ((AESSTATUS & 6) == 0);
        invalidate_dcache();
        if (!ptr) break;
        for (i = 0; i < 4; i++) ((uint32_t*)data)[ptr + i] ^= ((uint32_t*)data)[ptr + i - 4];
    }
    AESCONTROL = 0;
    PWRCONEXT |= 0x400;
}
