//
//
//    Copyright 2011 TheSeven
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


#ifndef __APP_BOOT_H__
#define __APP_BOOT_H__


#include "emcoreapp.h"


extern void run_powerdown(void** firmware, void** app, int* size);
extern void fastboot_rockbox(void** firmware, void** app, int* size);
extern void run_rockbox_fallback(void** firmware, void** app, int* size);
extern void run_rockbox(void** firmware, void** app, int* size);
extern void fastboot_umsboot(void** firmware, void** app, int* size);
extern void run_umsboot(void** firmware, void** app, int* size);


#endif
