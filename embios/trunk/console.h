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


#ifndef __CONSOLE_H__
#define __CONSOLE_H__


#include "global.h"
#include "libc/include/_ansi.h"


void console_init() INITCODE_ATTR;
void cputc(unsigned int consoles, char string) ICODE_ATTR;
void cputs(unsigned int consoles, const char* string) ICODE_ATTR;
void cwrite(unsigned int consoles, const char* string, size_t length) ICODE_ATTR;
int cprintf(unsigned int consoles, const char* fmt, ...) ICODE_ATTR
            ATTRIBUTE_PRINTF(2, 3);
int cvprintf(unsigned int consoles, const char* fmt, va_list ap) ICODE_ATTR;
void cflush(unsigned int consoles) ICODE_ATTR;
int cgetc(unsigned int consoles, int timeout) ICODE_ATTR;
int cread(unsigned int consoles, char* buffer, size_t length, int timeout) ICODE_ATTR;
void creada(unsigned int consoles, char* buffer, size_t length, int timeout) ICODE_ATTR;


#endif
