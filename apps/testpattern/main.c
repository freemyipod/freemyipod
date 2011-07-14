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


#include "emcoreapp.h"


static void main()
{
    int x, y, r;
    int width = lcd_get_width();
    int height = lcd_get_height();
    int l1, s1, ls1, ss1, l2, s2, ls2, ss2, x1, y1, x2, y2, ox1, oy1, ox2, oy2;
    if (width > height)
    {
        l1 = width;
        s1 = height;
    }
    else
    {
        l1 = height;
        s1 = width;
    }
    for (l2 = 0x40000000; l2 > l1; l2 >>= 1);
    if ((l2 * 9) / 8 > l1) l2 >>= 1;
    while ((l2 / 9) * 6 > s1) l2 >>= 1;
    l1 = (l2 * 9) / 8;
    s1 = l2 / 2;
    s2 = s1 / 2;
    if (height >= l1)
    {
        x1 = s1;
        x2 = s2;
        y1 = l1;
        y2 = l2;
        ls1 = 0;
        ls2 = 0;
        ss1 = 0;
        ss2 = 0;
        ox1 = (width - s1 - s2) / 2;
        ox2 = ox1 + s1;
        oy1 = (height - l1) / 2;
        oy2 = oy1;
    }
    else
    {
        x1 = l1;
        x2 = l2;
        y1 = s1;
        y2 = s2;
        ls1 = 3 - 3 * l1 * s1;
        ls2 = 3 - 3 * l2 * s2;
        ss1 = l1 * 3 - 3;
        ss2 = l2 * 3 - 3;
        ox1 = (width - l1) / 2;
        ox2 = ox1;
        oy1 = (height - s1 - s2) / 2;
        oy2 = oy1 + s1;
    }
    

    uint8_t* framebuf = (uint8_t*)malloc(s1 * l1 * 3);
    if (!framebuf) panicf(PANIC_KILLTHREAD, "Out of memory!");

    int s = s1 / 4;
    uint8_t* ptr = framebuf;
    for (r = 0; r < 8; r++)
        for (y = 0; y < s; y++)
        {
            for (x = 0; x < s; x++)
            {
                *ptr++ = 1 << r;
                *ptr++ = 0;
                *ptr++ = 0;
                ptr += ss1;
            }
            for (x = 0; x < s; x++)
            {
                *ptr++ = 0;
                *ptr++ = 1 << r;
                *ptr++ = 0;
                ptr += ss1;
            }
            for (x = 0; x < s; x++)
            {
                *ptr++ = 0;
                *ptr++ = 0;
                *ptr++ = 1 << r;
                ptr += ss1;
            }
            for (x = 0; x < s; x++)
            {
                *ptr++ = 1 << r;
                *ptr++ = 1 << r;
                *ptr++ = 1 << r;
                ptr += ss1;
            }
            ptr += ls1;
        }
    for (y = 0; y < s; y++)
    {
        for (x = 0; x < s; x++)
        {
            *ptr++ = 0xff;
            *ptr++ = 0;
            *ptr++ = 0;
            ptr += ss1;
        }
        for (x = 0; x < s; x++)
        {
            *ptr++ = 0;
            *ptr++ = 0xff;
            *ptr++ = 0;
            ptr += ss1;
        }
        for (x = 0; x < s; x++)
        {
            *ptr++ = 0;
            *ptr++ = 0;
            *ptr++ = 0xff;
            ptr += ss1;
        }
        for (x = 0; x < s; x++)
        {
            *ptr++ = 0xff;
            *ptr++ = 0xff;
            *ptr++ = 0xff;
            ptr += ss1;
        }
        ptr += ls1;
    }
    displaylcd(ox1, oy1, x1, y1, framebuf, 0, 0, x1);

    int sx = s2 / 8;
    int sy = MAX(1, l2 / 256);
    int i = 256 / (l2 / sy);
    s = 8;
    for (x = i; x > 1; x >>= 1) s--;
    ptr = framebuf;
    for (r = 0; r < 256; r += i)
        for (y = 0; y < sy; y++)
        {
            for (x = 0; x < sx; x++)
            {
                *ptr++ = ~(r | (r >> s));
                *ptr++ = ~(r | (r >> s));
                *ptr++ = ~(r | (r >> s));
                ptr += ss2;
            }
            for (x = 0; x < sx; x++)
            {
                *ptr++ = r | (r >> s);
                *ptr++ = r | (r >> s);
                *ptr++ = r | (r >> s);
                ptr += ss2;
            }
            for (x = 0; x < sx; x++)
            {
                *ptr++ = 0;
                *ptr++ = r | (r >> s);
                *ptr++ = r | (r >> s);
                ptr += ss2;
            }
            for (x = 0; x < sx; x++)
            {
                *ptr++ = r | (r >> s);
                *ptr++ = 0;
                *ptr++ = r | (r >> s);
                ptr += ss2;
            }
            for (x = 0; x < sx; x++)
            {
                *ptr++ = r | (r >> s);
                *ptr++ = r | (r >> s);
                *ptr++ = 0;
                ptr += ss2;
            }
            for (x = 0; x < sx; x++)
            {
                *ptr++ = r | (r >> s);
                *ptr++ = 0;
                *ptr++ = 0;
                ptr += ss2;
            }
            for (x = 0; x < sx; x++)
            {
                *ptr++ = 0;
                *ptr++ = r | (r >> s);
                *ptr++ = 0;
                ptr += ss2;
            }
            for (x = 0; x < sx; x++)
            {
                *ptr++ = 0;
                *ptr++ = 0;
                *ptr++ = r | (r >> s);
                ptr += ss2;
            }
            ptr += ls2;
        }
    displaylcd(ox2, oy2, x2, y2, framebuf, 0, 0, x2);

    free(framebuf);
}


EMCORE_APP_HEADER("LCD test pattern generator", main, 127)
