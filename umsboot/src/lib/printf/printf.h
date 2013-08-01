#ifndef __LIB_PRINTF_PRINTF_H__
#define __LIB_PRINTF_PRINTF_H__

#include "global.h"
#include <stdarg.h>

extern int printf_format(int (*push)(void *userp, unsigned char data), void *userp, const char *fmt, va_list ap);

#endif
