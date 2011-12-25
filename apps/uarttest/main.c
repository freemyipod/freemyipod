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


#include "emcoreapp.h"


#define ULCON  (*((uint32_t volatile*)0x3cc00000))
#define UCON   (*((uint32_t volatile*)0x3cc00004))
#define UFCON  (*((uint32_t volatile*)0x3cc00008))
#define UMCON  (*((uint32_t volatile*)0x3cc0000c))
#define UFSTAT (*((uint32_t volatile*)0x3cc00018))
#define UTXH   (*((uint8_t volatile*)0x3cc00020))
#define URXH   (*((uint8_t volatile*)0x3cc00024))
#define UBRDIV (*((uint32_t volatile*)0x3cc00028))


static void uart_tx(char byte)
{
    while (UFSTAT & BIT(9)) sleep(100);
    UTXH = byte;
}

static char uart_rx()
{
    while (!(UFSTAT & BITRANGE(0, 3))) sleep(100);
    return URXH;
}

static void main(int argc, const char** argv)
{
    int i;
    clockgate_enable(41, true);
    ULCON = 3;
    UCON = 0x405;
    UFCON = 7;
    UMCON = 0;
    switch (get_platform_id())
    {
    case 0x47324e49:  // IN2G
        UBRDIV = 38;
        break;
    case 0x4c435049:  // IPCL
        UBRDIV = 18;
        break;
    default:
        panic(PANIC_KILLTHREAD, "Unknown platform!");
    }
    uart_tx('~');
    while (true) uart_tx(uart_rx() ^ 0x20);
}


EMCORE_APP_HEADER("UART test", main, 127)
