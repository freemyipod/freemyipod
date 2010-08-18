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


#ifndef __HWKEYAES_H__
#define __HWKEYAES_H__

#include "global.h"


enum hwkeyaes_direction
{
    HWKEYAES_DECRYPT = 0,
    HWKEYAES_ENCRYPT = 1
};


void hwkeyaes(enum hwkeyaes_direction direction, uint32_t keyidx, void* data, uint32_t size);


#endif
