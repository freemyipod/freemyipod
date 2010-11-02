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


#ifndef __DEBUG_H__
#define __DEBUG_H__


#include "global.h"


#ifdef DEBUG_CONSOLES
#ifdef DEBUG_PRINT_SOURCE_LINE
#define DEBUGF(fmt, args...) cprintf(DEBUG_CONSOLES, "%s:%d: " fmt "\n", __FILE__, __LINE__, ##args)
#else
#define DEBUGF(fmt, args...) cprintf(DEBUG_CONSOLES, fmt "\n", ##args)
#endif
#else
#define DEBUGF(...)
#endif

#endif
