#include "global.h"
#include "interface/console/console.h"
#include "lib/printf/printf.h"


struct console_printf_data
{
    const struct console_instance* instance;
    int bytes;
};


void console_init(const struct console_instance* instance)
{
    instance->driver->init(instance);
}

int console_get_width(const struct console_instance* instance)
{
    if (!instance->driver->get_width) return -1;
    return instance->driver->get_width(instance);
}

int console_get_height(const struct console_instance* instance)
{
    if (!instance->driver->get_height) return -1;
    return instance->driver->get_height(instance);
}

void console_putc(const struct console_instance* instance, char c)
{
    instance->driver->putc(instance, c);
}

void console_puts(const struct console_instance* instance, const char* str)
{
    if (instance->driver->puts) return instance->driver->puts(instance, str);
    while (*str) instance->driver->putc(instance, *str++);
}

void console_write(const struct console_instance* instance, const char* buf, int len)
{
    if (instance->driver->write) return instance->driver->write(instance, buf, len);
    while (len--) instance->driver->putc(instance, *buf++);
}

int console_getc(const struct console_instance* instance, bool block)
{
    if (!instance->driver->getc) return -2;
    return instance->driver->getc(instance, block);
}

void console_flush(const struct console_instance* instance)
{
    instance->driver->flush(instance);
}

static int console_prfunc(void* ptr, unsigned char c)
{
    struct console_printf_data* pr = (struct console_printf_data*)ptr;
    console_putc(pr->instance, c);
    pr->bytes++;
    return true;
}

int console_vprintf(const struct console_instance* instance, const char* fmt, va_list ap)
{
    struct console_printf_data pr;
    pr.instance = instance;
    pr.bytes = 0;
    printf_format(console_prfunc, &pr, fmt, ap);
    return pr.bytes;
}

int console_printf(const struct console_instance* instance, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int bytes = console_vprintf(instance, fmt, ap);
    va_end(ap);
    return bytes;
}
