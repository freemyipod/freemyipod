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


#ifndef __APP_UTIL_H__
#define __APP_UTIL_H__


#include "emcoreapp.h"
#include "libpng.h"
#include "libui.h"


extern struct emcorelib_header* loadlib(uint32_t identifier, uint32_t version, char* filename);
extern void* loadpng(struct libpng_api* libpng, const char* buf, const uint32_t size,
                     void* (*decoder)(struct png_info* handle));
extern bool update_display(struct chooser_data* data);
extern void message(int x, const char* line1, const char* line2);


#endif
