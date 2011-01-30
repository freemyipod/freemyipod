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


#include "emcorelib.h"
#include "export/libpng.h"


struct libpng_api apitable =
{
    .png_open = png_open,
    .png_get_width = png_get_width,
    .png_get_height = png_get_height,
    .png_set_background = png_set_background,
    .png_decode_rgba = png_decode_rgba,
    .png_decode_rgb = png_decode_rgb,
    .png_destroy = png_destroy
};

EMCORE_LIB_HEADER(0x64474e50, LIBPNG_API_VERSION, LIBPNG_MIN_API_VERSION, NULL, NULL, apitable)
