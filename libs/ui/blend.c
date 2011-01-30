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
#include "blend.h"


void blend(int width, int height, int opacity,
           void* outbuf, int outx, int outy, int outstride,
           void* in1buf, int in1x, int in1y, int in1stride,
           void* in2buf, int in2x, int in2y, int in2stride)
{
    char* in1 = (char*)in1buf + (in1x + in1y * in1stride) * 3;
    char* in2 = (char*)in2buf + (in2x + in2y * in2stride) * 4;
    char* out = (char*)outbuf + (outx + outy * outstride) * 3;
    int x, y;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            int r1 = *in1++;
            int g1 = *in1++;
            int b1 = *in1++;
            int r2 = *in2++;
            int g2 = *in2++;
            int b2 = *in2++;
            int a = *in2++ * opacity;
            *out++ = (a * r2 + (65535 - a) * r1) >> 16;
            *out++ = (a * g2 + (65535 - a) * g1) >> 16;
            *out++ = (a * b2 + (65535 - a) * b1) >> 16;
        }
        in1 += (in1stride - width) * 3;
        in2 += (in2stride - width) * 4;
        out += (outstride - width) * 3;
    }
}
