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


#ifndef __DBGCONSOLE_H__
#define __DBGCONSOLE_H__


#include "global.h"


void dbgconsole_putc(char string) ICODE_ATTR;
void dbgconsole_puts(const char* string) ICODE_ATTR;
void dbgconsole_write(const char* string, size_t length) ICODE_ATTR;
int dbgconsole_getc(int timeout) ICODE_ATTR;
int dbgconsole_read(const char* string, size_t length, int timeout) ICODE_ATTR;


#endif
