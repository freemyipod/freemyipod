#ifndef __INTERFACE_CONSOLE_CONSOLE_H__
#define __INTERFACE_CONSOLE_CONSOLE_H__

#include "global.h"
#include <stdarg.h>


struct __attribute__((packed,aligned(4))) console_instance;


struct __attribute__((packed,aligned(4))) console_driver
{
    void (*init)(const struct console_instance* instance);
    int (*get_width)(const struct console_instance* instance);
    int (*get_height)(const struct console_instance* instance);
    void (*putc)(const struct console_instance* instance, char c);
    void (*puts)(const struct console_instance* instance, const char* str);
    void (*write)(const struct console_instance* instance, const char* buf, int len);
    void (*flush)(const struct console_instance* instance);
    int (*getc)(const struct console_instance* instance, bool block);
};


struct __attribute__((packed,aligned(4))) console_instance
{
    const struct console_driver* driver;
    const void* driver_config;
    void* driver_state;
};


extern void console_init(const struct console_instance* instance);
extern int console_get_width(const struct console_instance* instance);
extern int console_get_height(const struct console_instance* instance);
extern void console_putc(const struct console_instance* instance, char c);
extern void console_puts(const struct console_instance* instance, const char* str);
extern void console_write(const struct console_instance* instance, const char* buf, int len);
extern int console_getc(const struct console_instance* instance, bool block);
extern void console_flush(const struct console_instance* instance);
extern int console_vprintf(const struct console_instance* instance, const char* fmt, va_list ap);
extern int console_printf(const struct console_instance* instance, const char* fmt, ...);


#endif
