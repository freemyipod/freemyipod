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


#ifndef __PANIC_H__
#define __PANIC_H__


#include "global.h"
#include "libc/include/_ansi.h"
#include <stdarg.h>


enum panic_severity
{
    PANIC_KILLTHREAD = 0,
    PANIC_KILLUSERTHREADS = 1,
    PANIC_FATAL = 2
};


extern void hang();
extern void reset();


void panic(enum panic_severity severity, const char* string) ICODE_ATTR;
void panicf(enum panic_severity severity, const char* string, ...) ICODE_ATTR
            ATTRIBUTE_PRINTF(2, 3);


#endif
