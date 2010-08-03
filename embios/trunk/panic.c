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


#include "global.h"
#include "panic.h"
#include "lcd.h"
#include "lcdconsole.h"
#include "console.h"
#include "format.h"
#include "thread.h"
#include "contextswitch.h"


void hang();


void handle_panic(enum panic_severity severity)
{
    thread_exit();
}

void panic(enum panic_severity severity, const char* string)
{
    if (severity == PANIC_FATAL)
    {
        enter_critical_section();
        while (!displaylcd_safe());
        lcdconsole_puts_noblit("\n*PANIC*\n", 0, -1);
        lcdconsole_puts_noblit(string, 0, -1);
        lcdconsole_puts_noblit("\n", 0, -1);
        lcdconsole_update();
        hang();
    }
    else
    {
        cputs(1, "\n*PANIC*\n");
        cputs(1, string);
        cputc(1, '\n');
        handle_panic(severity);
    }
}

static int pprfunc(void* ptr, unsigned char letter)
{
    lcdconsole_putc_noblit(letter, 0, -1);
    return true;
}

void panicf(enum panic_severity severity, const char* string, ...)
{
    va_list ap;
    if (severity == PANIC_FATAL)
    {
        enter_critical_section();
        while (!displaylcd_safe());
        lcdconsole_puts_noblit("\n*PANIC*\n", 0, -1);
        va_start(ap, string);
        format(pprfunc, NULL, string, ap);
        va_end(ap);
        lcdconsole_puts_noblit("\n", 0, -1);
        lcdconsole_update();
        hang();
    }
    else
    {
        cputs(1, "\n*PANIC*\n");
        va_start(ap, string);
        cvprintf(1, string, ap);
        va_end(ap);
        cputc(1, '\n');
        handle_panic(severity);
    }
}

void __div0()
{
    panic(PANIC_KILLTHREAD, "Division by zero!");
}
