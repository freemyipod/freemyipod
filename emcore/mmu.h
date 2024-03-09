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


#ifndef __MMU_H__
#define __MMU_H__


#include "global.h"


void clean_dcache() __attribute__((naked, noinline)) ICODE_ATTR;
void invalidate_dcache() __attribute__((naked, noinline)) ICODE_ATTR;
void invalidate_icache() __attribute__((naked, noinline)) ICODE_ATTR;


#endif
