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
#include "util.h"


void* memcpy(void* destination, const void* source, size_t num)
{
    unsigned char* dest = (unsigned char*)destination;
    unsigned char* src = (unsigned char*)source;
    while (num--) *dest++ = *src++;
    return destination;
}

void* memset(void* ptr, int value, size_t num)
{
    unsigned char* dest = (unsigned char*)ptr;
    while (num--) *dest++ = (unsigned char)value;
    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num)
{
    unsigned char* src1 = (unsigned char*)ptr1;
    unsigned char* src2 = (unsigned char*)ptr2;
    int diff;
    while (num--)
      if (diff = *src1++ - *src2++)
        return diff;
    return 0;
}
