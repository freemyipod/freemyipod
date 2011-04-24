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
#include "console.h"
#include "format.h"
#include "thread.h"
#include "contextswitch.h"


void handle_panic(enum panic_severity severity, uint32_t mode)
{
    if (severity == PANIC_FATAL) panic_fatal(mode);
    struct scheduler_thread* t;
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
    panic_recover(mode);
}

void panic(enum panic_severity severity, const char* string)
{
    uint32_t mode = enter_critical_section();
    csputs(CONSOLE_PANIC, "\n*PANIC*\n");
    csputs(CONSOLE_PANIC, string);
    csputc(CONSOLE_PANIC, '\n');
    csflush(CONSOLE_PANIC);
    handle_panic(severity, mode);
}

void panicf(enum panic_severity severity, const char* string, ...)
{
    va_list ap;
    uint32_t mode = enter_critical_section();
    csputs(CONSOLE_PANIC, "\n*PANIC*\n");
    va_start(ap, string);
    csvprintf(CONSOLE_PANIC, string, ap);
    va_end(ap);
    csputc(CONSOLE_PANIC, '\n');
    csflush(CONSOLE_PANIC);
    handle_panic(severity, mode);
}

void __div0()
{
    panic(PANIC_KILLTHREAD, "Division by zero!");
}
