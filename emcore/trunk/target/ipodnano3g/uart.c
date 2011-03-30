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


#include "global.h"
#include "uart.h"
#include "s5l8702.h"
#include "timer.h"


void uart_init()
{
    clockgate_enable(41, true);
    ULCON = 3;
    UCON = 0x405;
    UFCON = 7;
    UMCON = 0;
    uart_set_baud(250000);
}

void uart_set_baud(int baud)
{
    UBRDIV = (750000 / baud) - 1;
}

void uart_putc(char byte)
{
    while (UFSTAT & BIT(9)) sleep(100);
    UTXH = byte;
}

void uart_puts(const char* string)
{
    char byte;
    while (byte = *string++) uart_putc(byte);
}

void uart_write(const char* string, size_t length)
{
    while (length--) uart_putc(*string++);
}

int uart_getc(int timeout)
{
    int byte = -1;
    long starttime = USEC_TIMER;
    while (!(UFSTAT & BITRANGE(0, 3)) && !TIMEOUT_EXPIRED(starttime, timeout)) sleep(100);
    if (UFSTAT & BITRANGE(0, 3)) byte = URXH;
    return byte;
}

int uart_read(char* string, size_t length, int timeout)
{
    int count = 0;
    long starttime = USEC_TIMER;
    while (length && !TIMEOUT_EXPIRED(starttime, timeout))
    {
        if (UFSTAT & BITRANGE(0, 3))
        {
            *string++ = URXH;
            length--;
            count++;
        }
        else sleep(100);
    }
    return count;
}
