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
#include "console.h"
#include "lcdconsole.h"
#include "format.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>


struct for_cprintf
{
    unsigned int consoles;
    size_t bytes;
};


static int cprfunc(void* ptr, unsigned char letter)
{
    struct for_cprintf* pr = (struct for_cprintf*)ptr;
    cputc_noblit(pr->consoles, letter);
    pr->bytes++;
    return true;
}

int cprintf(unsigned int consoles, const char* fmt, ...)
{
    va_list ap;
    struct for_cprintf pr;

    pr.consoles = consoles;
    pr.bytes = 0;

    va_start(ap, fmt);
    format(cprfunc, &pr, fmt, ap);
    va_end(ap);
    
    lcdconsole_update();

    return pr.bytes;
}

int cvprintf(unsigned int consoles, const char* fmt, va_list ap)
{
    struct for_cprintf pr;

    pr.consoles = consoles;
    pr.bytes = 0;

    format(cprfunc, &pr, fmt, ap);
    
    lcdconsole_update();

    return pr.bytes;
}

void cputc(unsigned int consoles, char string)
{
  if (consoles & 1) lcdconsole_putc(string, 0, -1);
}

void cputc_noblit(unsigned int consoles, char string)
{
  if (consoles & 1) lcdconsole_putc_noblit(string, 0, -1);
}

void cputs(unsigned int consoles, const char* string)
{
  if (consoles & 1) lcdconsole_puts(string, 0, -1);
}

void cputs_noblit(unsigned int consoles, const char* string)
{
  if (consoles & 1) lcdconsole_puts_noblit(string, 0, -1);
}

void cflush(unsigned int consoles)
{
  if (consoles & 1) lcdconsole_update();
}
