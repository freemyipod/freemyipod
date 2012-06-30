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
#include "console.h"
#include "lcdconsole.h"
#include "usb/dbgconsole.h"
#include "uart.h"
#include "format.h"
#include "thread.h"
#include <stdarg.h>
#include <limits.h>


struct for_cprintf
{
    unsigned int consoles;
    size_t bytes;
};


struct mutex console_mutex;
struct mutex console_readmutex;


void console_init()
{
    mutex_init(&console_mutex);
    mutex_init(&console_readmutex);
#ifdef HAVE_UART
    uart_init();
#endif
}

void console_set_speed(unsigned int consoles, unsigned int cps)
{
#ifdef HAVE_UART
    if (consoles & 4) uart_set_baud(cps * 10);
#endif
}

void cputc_internal(unsigned int consoles, char string) ICODE_ATTR;
void cputc_internal(unsigned int consoles, char string)
{
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_putc_noblit(string, LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
#endif
#ifdef HAVE_USB
    if (consoles & 2) dbgconsole_putc(string);
#endif
#ifdef HAVE_UART
    if (consoles & 4) uart_putc(string);
#endif
}

static int cprfunc(void* ptr, unsigned char letter)
{
    struct for_cprintf* pr = (struct for_cprintf*)ptr;
    cputc_internal(pr->consoles, letter);
    pr->bytes++;
    return true;
}

static int csprfunc(void* ptr, unsigned char letter)
{
    struct for_cprintf* pr = (struct for_cprintf*)ptr;
    csputc(pr->consoles, letter);
    pr->bytes++;
    return true;
}

int cprintf(unsigned int consoles, const char* fmt, ...)
{
    va_list ap;
    struct for_cprintf pr;

    pr.consoles = consoles;
    pr.bytes = 0;

    mutex_lock(&console_mutex, TIMEOUT_BLOCK);
    va_start(ap, fmt);
    format(cprfunc, &pr, fmt, ap);
    va_end(ap);
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_update();
#endif
    mutex_unlock(&console_mutex);

    return pr.bytes;
}

int csprintf(unsigned int consoles, const char* fmt, ...)

{
    va_list ap;
    struct for_cprintf pr;

    pr.consoles = consoles;
    pr.bytes = 0;

    uint32_t mode = enter_critical_section();
    va_start(ap, fmt);
    format(csprfunc, &pr, fmt, ap);
    va_end(ap);
    leave_critical_section(mode);

    return pr.bytes;
}

int cvprintf(unsigned int consoles, const char* fmt, va_list ap)
{
    struct for_cprintf pr;

    pr.consoles = consoles;
    pr.bytes = 0;

    mutex_lock(&console_mutex, TIMEOUT_BLOCK);
    format(cprfunc, &pr, fmt, ap);
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_update();
#endif
    mutex_unlock(&console_mutex);

    return pr.bytes;
}

int csvprintf(unsigned int consoles, const char* fmt, va_list ap)
{
    struct for_cprintf pr;

    pr.consoles = consoles;
    pr.bytes = 0;

    uint32_t mode = enter_critical_section();
    format(csprfunc, &pr, fmt, ap);
    leave_critical_section(mode);

    return pr.bytes;
}

void cputc(unsigned int consoles, char string)
{
    mutex_lock(&console_mutex, TIMEOUT_BLOCK);
    cputc_internal(consoles, string);
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_update();
#endif
    mutex_unlock(&console_mutex);
}

void csputc(unsigned int consoles, char string)
{
    uint32_t mode = enter_critical_section();
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_putc_noblit(string, LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
#endif
#ifdef HAVE_USB
    if (consoles & 2) dbgconsole_sputc(string);
#endif
#ifdef HAVE_UART
    if (consoles & 4) uart_sputc(string);
#endif
    leave_critical_section(mode);
}

void cputs(unsigned int consoles, const char* string)
{
    mutex_lock(&console_mutex, TIMEOUT_BLOCK);
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_puts(string, LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
#endif
#ifdef HAVE_USB
    if (consoles & 2) dbgconsole_puts(string);
#endif
#ifdef HAVE_UART
    if (consoles & 4) uart_puts(string);
#endif
    mutex_unlock(&console_mutex);
}

void csputs(unsigned int consoles, const char* string)
{
    uint32_t mode = enter_critical_section();
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_puts_noblit(string, LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
#endif
#ifdef HAVE_USB
    if (consoles & 2) dbgconsole_sputs(string);
#endif
#ifdef HAVE_UART
    if (consoles & 4) uart_sputs(string);
#endif
    leave_critical_section(mode);
}

void cwrite(unsigned int consoles, const char* string, size_t length)
{
    mutex_lock(&console_mutex, TIMEOUT_BLOCK);
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_write(string, length, LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
#endif
#ifdef HAVE_USB
    if (consoles & 2) dbgconsole_write(string, length);
#endif
#ifdef HAVE_UART
    if (consoles & 4) uart_write(string, length);
#endif
    mutex_unlock(&console_mutex);
}

void cswrite(unsigned int consoles, const char* string, size_t length)
{
    uint32_t mode = enter_critical_section();
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_write_noblit(string, length, LCDCONSOLE_FGCOLOR, LCDCONSOLE_BGCOLOR);
#endif
#ifdef HAVE_USB
    if (consoles & 2) dbgconsole_swrite(string, length);
#endif
#ifdef HAVE_UART
    if (consoles & 4) uart_swrite(string, length);
#endif
    leave_critical_section(mode);
}

void cflush(unsigned int consoles)
{
    mutex_lock(&console_mutex, TIMEOUT_BLOCK);
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_update();
#endif
    mutex_unlock(&console_mutex);
}

void csflush(unsigned int consoles)
{
    uint32_t mode = enter_critical_section();
#ifdef HAVE_LCD
    if (consoles & 1) lcdconsole_supdate();
#endif
    leave_critical_section(mode);
}

static inline int cread_unlock_return(unsigned int rc)
{
    mutex_unlock(&console_readmutex);
    return rc;
}

int cgetc(unsigned int consoles, int timeout)
{
    int result;
    mutex_lock(&console_readmutex, TIMEOUT_BLOCK);
#ifdef HAVE_USB
    if ((consoles & 2) && (result = dbgconsole_getc(timeout)) != -1) return cread_unlock_return(result);
#endif
#ifdef HAVE_UART
    if ((consoles & 4) && (result = uart_getc(timeout)) != -1) return cread_unlock_return(result);
#endif
    return cread_unlock_return(-1);
}

int cread(unsigned int consoles, char* buffer, size_t length, int timeout)
{
    int result;
    mutex_lock(&console_readmutex, TIMEOUT_BLOCK);
#ifdef HAVE_USB
    if ((consoles & 2) && (result = dbgconsole_read(buffer, length, timeout))) return cread_unlock_return(result);
#endif
#ifdef HAVE_UART
    if ((consoles & 4) && (result = uart_read(buffer, length, timeout))) return cread_unlock_return(result);
#endif
    return cread_unlock_return(-1);
}

void creada(unsigned int consoles, char* buffer, size_t length, int timeout)
{
    int result;
    mutex_lock(&console_readmutex, TIMEOUT_BLOCK);
    while (length)
    {
        if (
#ifdef HAVE_USB
            ((consoles & 2) && (result = dbgconsole_read(buffer, length, timeout)))
#else
            false
#endif
            ||
#ifdef HAVE_UART
            ((consoles & 4) && (result = uart_read(buffer, length, timeout)))
#else
            false
#endif
            )
        {
            buffer = &buffer[result];
            length -= result;
        }
    }
    mutex_unlock(&console_readmutex);
}
