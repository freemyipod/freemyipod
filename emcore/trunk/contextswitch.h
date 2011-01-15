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


#ifndef __CONTEXTSWITCH_H__
#define __CONTEXTSWITCH_H__


#include "global.h"


void handle_irq(void) __attribute__((noreturn)) ICODE_ATTR;
void context_switch(void) ICODE_ATTR;
void resume_thread(void) __attribute__((noreturn)) ICODE_ATTR;
uint32_t enter_critical_section(void) ICODE_ATTR;
void leave_critical_section(uint32_t mode) ICODE_ATTR;
void execfirmware(void* addr) ICODE_ATTR;


#endif
