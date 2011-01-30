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


#include "global.h"
#include "panic.h"
#include "lcd.h"
#include "lcdconsole.h"
#include "console.h"
#include "format.h"
#include "thread.h"
#include "contextswitch.h"


void handle_panic(enum panic_severity severity)
{
    struct scheduler_thread* t;
    uint32_t mode = enter_critical_section();
    if (severity == PANIC_KILLUSERTHREADS)
    {
        int i;
        for (t = head_thread; t; t = t->thread_next)
            if (t->type == USER_THREAD)
                t->state = THREAD_SUSPENDED;
    }
    current_thread->state = THREAD_DEFUNCT_ACK;
    current_thread->block_type = THREAD_DEFUNCT_PANIC;
    leave_critical_section(mode);
    context_switch();
}

void panic(enum panic_severity severity, const char* string)
{
    if (severity == PANIC_FATAL)
    {
        enter_critical_section();
        while (!displaylcd_safe());
        lcdconsole_puts_noblit("\n*PANIC*\n", LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
        lcdconsole_puts_noblit(string, LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
        lcdconsole_puts_noblit("\n", LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
        lcdconsole_update();
        hang();
    }
    else
    {
        cputs(CONSOLE_BOOT, "\n*PANIC*\n");
        cputs(CONSOLE_BOOT, string);
        cputc(CONSOLE_BOOT, '\n');
        handle_panic(severity);
    }
}

static int pprfunc(void* ptr, unsigned char letter)
{
    lcdconsole_putc_noblit(letter, LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
    return true;
}

void panicf(enum panic_severity severity, const char* string, ...)
{
    va_list ap;
    if (severity == PANIC_FATAL)
    {
        enter_critical_section();
        while (!displaylcd_safe());
        lcdconsole_puts_noblit("\n*PANIC*\n", LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
        va_start(ap, string);
        format(pprfunc, NULL, string, ap);
        va_end(ap);
        lcdconsole_puts_noblit("\n", LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
        lcdconsole_update();
        hang();
    }
    else
    {
        cputs(CONSOLE_BOOT, "\n*PANIC*\n");
        va_start(ap, string);
        cvprintf(CONSOLE_BOOT, string, ap);
        va_end(ap);
        cputc(CONSOLE_BOOT, '\n');
        handle_panic(severity);
    }
}

void __div0()
{
    panic(PANIC_KILLTHREAD, "Division by zero!");
}
