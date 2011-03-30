//
//
//    Copyright 2011 TheSeven
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


#ifndef __UART_H__
#define __UART_H__


#include "global.h"


void uart_init() INITCODE_ATTR;
void uart_set_baud(int baud);
void uart_putc(char string) ICODE_ATTR;
void uart_puts(const char* string) ICODE_ATTR;
void uart_write(const char* string, size_t length) ICODE_ATTR;
int uart_getc(int timeout) ICODE_ATTR;
int uart_read(char* string, size_t length, int timeout) ICODE_ATTR;


#endif
