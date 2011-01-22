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


#ifndef __LIBBOOT_H__
#define __LIBBOOT_H__

#include "emcorelib.h"


int verify_rockbox_checksum(void* image, size_t size);


/* increase this every time the api struct changes */
#define LIBBOOT_API_VERSION 1

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define LIBBOOT_MIN_API_VERSION 1

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase LIBBOOT_API_VERSION. If you make changes to the
         existing APIs, also update LIBBOOT_MIN_API_VERSION to current version */

struct libboot_api
{
    typeof(verify_rockbox_checksum)* verify_rockbox_checksum;
};


#endif
