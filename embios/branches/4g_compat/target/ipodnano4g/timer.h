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


#ifndef __TIMER_H__
#define __TIMER_H__

#include "global.h"


#define TIME_AFTER(a,b)         ((long)(b) - (long)(a) < 0)
#define TIME_BEFORE(a,b)        TIME_AFTER(b,a)
#define TIMEOUT_EXPIRED(a,b)    TIME_AFTER(USEC_TIMER,a + b)


static inline void sleep(long duration)  /* in usec steps */
{
    long timestamp = USEC_TIMER;
    while (!TIMEOUT_EXPIRED(timestamp, duration));
}


#endif
