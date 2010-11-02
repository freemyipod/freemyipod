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


#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__


#include "global.h"


void irqhandler(void) ICODE_ATTR;
void interrupt_init(void) INITCODE_ATTR;
void interrupt_shutdown(void);
void interrupt_enable(int irq, bool state);
void interrupt_set_handler(int irq, void* handler);
void int_timer_set_handler(int timer, void* handler);

#endif
