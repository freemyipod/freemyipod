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


#ifndef __BLEND_H__
#define __BLEND_H__

#include "emcorelib.h"


void blend(int width, int height, int opacity,
           void* outbuf, int outx, int outy, int outstride,
           void* in1buf, int in1x, int in1y, int in1stride,
           void* in2buf, int in2x, int in2y, int in2stride);
void blendcolor(int width, int height, uint32_t color,
                void* outbuf, int outx, int outy, int outstride,
                void* inbuf, int inx, int iny, int instride);
void mattecolor(int width, int height, uint32_t color,
                void* outbuf, int outx, int outy, int outstride,
                void* inbuf, int inx, int iny, int instride);


#endif
