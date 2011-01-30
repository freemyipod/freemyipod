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


#ifndef __PNG_H__
#define __PNG_H__


#include "emcorelib.h"


struct png_rgb
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} __attribute__((packed));

struct png_rgba
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} __attribute__((packed));

struct png_info
{
    uint32_t width;
    uint32_t height;
    uint8_t depth;
    uint8_t colortype;
    uint8_t comprtype;
    uint8_t filtertype;
    const struct png_rgb* palette;
    const uint8_t* palalpha;
    uint32_t palalphacnt;
    const void* idat;
    size_t idatlen;
    uint16_t key[3];
    struct png_rgb bg;
    uint8_t keyvalid;
    uint8_t channels;
} __attribute__((packed));


struct png_info* png_open(const void* data, size_t size);
uint32_t png_get_width(struct png_info* info);
uint32_t png_get_height(struct png_info* info);
void png_set_background(struct png_info* info, uint32_t color);
struct png_rgba* png_decode_rgba(struct png_info* info);
struct png_rgb* png_decode_rgb(struct png_info* info);
void png_destroy(struct png_info* info);


#endif
