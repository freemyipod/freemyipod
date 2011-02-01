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


#ifndef __EXPORT_LIBPNG_H__
#define __EXPORT_LIBPNG_H__


#include "emcorelib.h"


#include "../png.h"


/* increase this every time the api struct changes */
#define LIBPNG_API_VERSION 1

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define LIBPNG_MIN_API_VERSION 1

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase LIBPNG_API_VERSION. If you make changes to the
         existing APIs, also update LIBPNG_MIN_API_VERSION to current version */

struct libpng_api
{
    typeof(png_open)* png_open;
    typeof(png_get_width)* png_get_width;
    typeof(png_get_height)* png_get_height;
    typeof(png_set_background)* png_set_background;
    typeof(png_decode_rgba)* png_decode_rgba;
    typeof(png_decode_rgb)* png_decode_rgb;
    typeof(png_destroy)* png_destroy;
};


#endif
