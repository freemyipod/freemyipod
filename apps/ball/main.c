//
//
//    Copyright 2011 user890104
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

#define BALL_W 5
#define BALL_H 5

unsigned int dw, dh, dbpp;
void* fb;

static inline void drawat(unsigned int x, unsigned int y, unsigned char color)
{
    if (x >= dw || y >= dh) return;
    
    // TODO: is there a better way?
    *((char *)(fb + dbpp * x + dbpp * dw * y)) = color;
    *((char *)(fb + dbpp * x + dbpp * dw * y + 1)) = color;
    *((char *)(fb + dbpp * x + dbpp * dw * y + 2)) = color;
}

static void main()
{
    unsigned int run_cycles = 5000;
    
    dw = lcd_get_width();
    dh = lcd_get_height();
    dbpp = 3; // 24-bit FB (rgb888)
    
    unsigned int fb_size = dbpp * dw * dh;
    fb = malloc(fb_size);
    
    if (fb == NULL)
    {
        panic(PANIC_KILLTHREAD, "Unable to allocate framebuffer!");
    }
    
    unsigned int i, x = 0, y = 0,
    old_x, old_y, bx, by,
    size = BALL_W * BALL_H;
    int vx = 1, vy = 1;
    
    //filllcd(0, 0, dw, dh, 0xffffff); // broken on Nano 4G?
    memset(fb, 0xff, fb_size);
    displaylcd(0, 0, dw, dh, fb, 0, 0, dw);
    
    while (run_cycles > 0)
    {
        memset(fb, 0xff, fb_size);
        
        // check if we hit a wall
        if ((x >= dw - BALL_W && vx > 0) || (x == 0 && vx < 0))
        {
            vx = -vx;
        }
        
        if ((y >= dh - BALL_H && vy > 0) || (y == 0 && vy < 1))
        {
            vy = -vy;
        }
        
        // store old x and y
        // needed for redrawing later
        old_x = x; old_y = y;
        
        x += vx; y += vy;
        
        // draw our ball-like object
        // a circle-like actually, since we
        // don't have a 3D engine yet :)
        for (i = 1; i < BALL_W - 1; ++i)
        {
            drawat(x + i, y, 0);
        }
        
        for (i = BALL_W; i < size - BALL_W; ++i)
        {
            drawat(x + i % BALL_W, y + i / BALL_W, 0);
        }
        
        for (i = 1; i < BALL_W - 1; ++i)
        {
            drawat(x + i, y + BALL_H - 1, 0);
        }
        
        // offset to redraw from
        bx = vx > 0 ? old_x : x;
        by = vy > 0 ? old_y : y;
        
        displaylcd(bx, by, BALL_W + ABS(vx), BALL_H + ABS(vy), fb, bx, by, dw);
        
        sleep(1000);
        
        --run_cycles;
    }
    
    cputs(3, "Application terminated\n");
}


EMCORE_APP_HEADER("Ball", main, 127)
