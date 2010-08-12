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


#ifndef __SYSCALLAPI_H__
#define __SYSCALLAPI_H__


#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include "../panic.h"


/* increase this every time the api struct changes */
#define EMBIOS_API_VERSION 0

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define EMBIOS_MIN_API_VERSION 0

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase EMBIOS_API_VERSION. If you make changes to the
         existing APIs, also update EMBIOS_MIN_API_VERSION to current version */

struct embios_syscall_table
{
    uint32_t table_version;
    uint32_t table_minversion;
	void (*panic) (enum panic_severity severity, const char* string);
	void (*panicf) (enum panic_severity severity, const char* fmt, ...);
};


#endif
