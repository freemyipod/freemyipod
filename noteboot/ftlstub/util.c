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
#include <util.h>


void* memcpy(void* destination, const void* source, size_t size)
{
    while (size > 0)
    {
        ((uint8_t*)destination)[0] = ((uint8_t*)source)[0];
        source = (void*)((uint32_t)source + 1);
        destination = (void*)((uint32_t)destination + 1);
        size--;
    }
    return destination;
}

void* memset(void* destination, int value, size_t size)
{
    while (size > 0)
    {
        ((uint8_t*)destination)[0] = value;
        destination = (void*)((uint32_t)destination + 1);
        size--;
    }
    return destination;
}

int memcmp(const void* destination, const void* source, size_t size)
{
    while (size > 0)
    {
        if (((uint8_t*)destination)[0] != ((uint8_t*)source)[0]) return 1;
        source = (void*)((uint32_t)source + 1);
        destination = (void*)((uint32_t)destination + 1);
        size--;
    }
    return 0;
}
