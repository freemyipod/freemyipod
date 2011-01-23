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
#include "dither.h"


static void dither_slow(int width, int height, void* inbuf, int inx, int iny, int instride,
                        void* outbuf, int outx, int outy, int outstride)
{
    int bpp = lcd_get_bytes_per_pixel();
    int bits = lcd_get_format();
    int swap = bits & BIT(30);
    int rwidth = MAX(7, bits & BITRANGE(0, 3));
    int gwidth = MAX(7, bits & BITRANGE(10, 13));
    int bwidth = MAX(7, bits & BITRANGE(20, 23));
    int rshift = bits & BITRANGE(4, 9);
    int gshift = bits & BITRANGE(14, 19);
    int bshift = bits & BITRANGE(24, 29);
    int roffs = 1 << (6 - rwidth);
    int goffs = 1 << (6 - gwidth);
    int boffs = 1 << (6 - bwidth);
    int rmask = BITRANGE(7 - rwidth, 7);
    int gmask = BITRANGE(7 - gwidth, 7);
    int bmask = BITRANGE(7 - bwidth, 7);
    int rclip = 7 - rshift;
    int gclip = 7 - gshift;
    int bclip = 7 - bshift;
    char* in = ((char*)inbuf) + (instride * iny + inx) * 3;
    char* out = ((char*)outbuf) + (instride * iny + inx) * bpp;
    int x, y;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            int origb = *in++;
            int origg = *in++;
            int origr = *in++;
            int realr = origr >> rclip;
            int realg = origg >> gclip;
            int realb = origb >> bclip;
            int errr = origr - (realr << rclip) - roffs;
            int errg = origg - (realg << gclip) - goffs;
            int errb = origb - (realb << bclip) - boffs;
            if (x + 1 < width)
            {
                *(in + 0) = MAX(0, MIN(255, *(in + 0) + errb / 2));
                *(in + 1) = MAX(0, MIN(255, *(in + 1) + errg / 2));
                *(in + 2) = MAX(0, MIN(255, *(in + 2) + errr / 2));
            }
            if (y + 1 < height)
            {
                *(in + 3 * instride - 1) = MAX(0, MIN(255, *(in + 3 * instride - 1) + errr / 4));
                *(in + 3 * instride - 2) = MAX(0, MIN(255, *(in + 3 * instride - 2) + errg / 4));
                *(in + 3 * instride - 3) = MAX(0, MIN(255, *(in + 3 * instride - 3) + errb / 4));
                *(in + 3 * instride - 4) = MAX(0, MIN(255, *(in + 3 * instride - 4) + errr / 4));
                *(in + 3 * instride - 5) = MAX(0, MIN(255, *(in + 3 * instride - 5) + errg / 4));
                *(in + 3 * instride - 6) = MAX(0, MIN(255, *(in + 3 * instride - 6) + errb / 4));
            }
            int pixel = (realr << rshift) | (realg << gshift) | (realb << bshift);
            if (bpp == 1) *out = pixel;
            else if (bpp == 2)
            {
                if (swap) *((short*)out) = (pixel >> 8) | ((pixel << 8) & 0xff00);
                else *((short*)out) = pixel;
            }
            else if (bpp == 3)
            {
                if (swap)
                {
                    *(out + 0) = pixel & 0xff;
                    *(out + 1) = (pixel >> 8) & 0xff;
                    *(out + 2) = (pixel >> 16) & 0xff;
                }
                else
                {
                    *(out + 0) = (pixel >> 16) & 0xff;
                    *(out + 1) = (pixel >> 8) & 0xff;
                    *(out + 2) = pixel & 0xff;
                }
            }
            else if (bpp == 4)
            {
                if (swap) *((int*)out) = (pixel >> 24) | ((pixel >> 8) & 0xff00)
                                       | ((pixel << 8) & 0xff0000) | ((pixel << 24) & 0xff000000);
                else *((int*)out) = pixel;
            }
            out += bpp;
        }
        in += (instride - width) * 3;
        out += (outstride - width) * bpp;
    }
}

static void dither_rgb565(int width, int height, void* inbuf, int inx, int iny, int instride,
                          void* outbuf, int outx, int outy, int outstride)
{
    char* in = ((char*)inbuf) + (instride * iny + inx) * 3;
    short* out = ((short*)outbuf) + instride * iny + inx;
    int x, y;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            int origb = *in++;
            int origg = *in++;
            int origr = *in++;
            int realr = origr >> 3;
            int realg = origg >> 2;
            int realb = origb >> 3;
            int errr = origr - (realr << 3) - 4;
            int errg = origg - (realg << 2) - 2;
            int errb = origb - (realb << 3) - 4;
            if (x + 1 < width)
            {
                *(in + 0) = MAX(0, MIN(255, *(in + 0) + errb / 2));
                *(in + 1) = MAX(0, MIN(255, *(in + 1) + errg / 2));
                *(in + 2) = MAX(0, MIN(255, *(in + 2) + errr / 2));
            }
            if (y + 1 < height)
            {
                *(in + 3 * instride - 1) = MAX(0, MIN(255, *(in + 3 * instride - 1) + errr / 4));
                *(in + 3 * instride - 2) = MAX(0, MIN(255, *(in + 3 * instride - 2) + errg / 4));
                *(in + 3 * instride - 3) = MAX(0, MIN(255, *(in + 3 * instride - 3) + errb / 4));
                *(in + 3 * instride - 4) = MAX(0, MIN(255, *(in + 3 * instride - 4) + errr / 4));
                *(in + 3 * instride - 5) = MAX(0, MIN(255, *(in + 3 * instride - 5) + errg / 4));
                *(in + 3 * instride - 6) = MAX(0, MIN(255, *(in + 3 * instride - 6) + errb / 4));
            }
            *out++ = (realr << 11) | (realg << 5) | realb;
        }
        in += (instride - width) * 3;
        out += outstride - width;
    }
}


void dither(int width, int height, void* inbuf, int inx, int iny, int instride,
            void* outbuf, int outx, int outy, int outstride)
{
    int bits = lcd_get_format();
    if (bits == 0x004154b1)
        dither_rgb565(width, height, inbuf, inx, iny, instride, outbuf, outx, outy, outstride);
    else dither_slow(width, height, inbuf, inx, iny, instride, outbuf, outx, outy, outstride);
}
