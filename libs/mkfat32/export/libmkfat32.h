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


#ifndef __EXPORT_LIBMKFAT32_H__
#define __EXPORT_LIBMKFAT32_H__

#include "emcorelib.h"


int mkfat32(int volume, int startsector, int totalsectors, int sectorsize, int secperclus,
            const char* label, void* statususer, void (*statusinit)(void* user, int max),
            void (*statuscallback)(void* user, int current));


/* emCORE library identifier */
#define LIBMKFAT32_IDENTIFIER 0x3233464d

/* increase this every time the api struct changes */
#define LIBMKFAT32_API_VERSION 1

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define LIBMKFAT32_MIN_API_VERSION 1

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase LIBMKFAT32_API_VERSION. If you make changes to the
         existing APIs, also update LIBMKFAT32_MIN_API_VERSION to current version */

struct libmkfat32_api
{
    typeof(mkfat32)* mkfat32;
};


#endif
