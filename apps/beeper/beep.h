//
//
//    Copyright 2011 TheSeven, user890104
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


#ifndef __BEEP_H__
#define __BEEP_H__

#include "emcoreapp.h"

/////TIMER/////
#define TDCON        (*((uint32_t volatile*)(0x3C700060)))
#define TDCMD        (*((uint32_t volatile*)(0x3C700064)))
#define TDDATA0      (*((uint32_t volatile*)(0x3C700068)))
#define TDDATA1      (*((uint32_t volatile*)(0x3C70006C)))
#define TDPRE        (*((uint32_t volatile*)(0x3C700070)))
#define TDCNT        (*((uint32_t volatile*)(0x3C700074)))

void singlebeep(unsigned int cycles, unsigned int time);

#endif
