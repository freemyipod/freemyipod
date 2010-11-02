//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include "global.h"
#include "progressbar.h"
#include "lcd.h"


static void progressbar_drawborderbg(struct progressbar_state* state, int bgstart)
{
    state->currentx = state->startx;
    displaylcd(bgstart + 1, state->endx - 1, state->starty + 1, state->endy - 1,
               (void*)-1, state->bgcolor);
    displaylcd(state->startx, state->endx, state->starty, state->starty,
               (void*)-1, state->bordercolor);
    displaylcd(state->startx, state->endx, state->endy, state->endy,
               (void*)-1, state->bordercolor);
    displaylcd(state->startx, state->startx, state->starty + 1, state->endy - 1,
               (void*)-1, state->bordercolor);
    displaylcd(state->endx, state->endx, state->starty + 1, state->endy - 1,
               (void*)-1, state->bordercolor);
}

void progressbar_init(struct progressbar_state* state, int startx, int endx, int starty, int endy,
                      int bordercolor, int bgcolor, int fgcolor, int min, int max)
{
    state->startx = startx;
    state->endx = endx;
    state->starty = starty;
    state->endy = endy;
    state->bordercolor = bordercolor;
    state->bgcolor = bgcolor;
    state->fgcolor = fgcolor;
    state->min = min;
    state->max = max;
    progressbar_drawborderbg(state, state->startx);
}

void progressbar_setpos(struct progressbar_state* state, int value, bool redraw)
{
    if (value > state->max) value = state->max;
    if (value < state->min) value = state->min;
    int newx = (value - state->min) * (state->endx - state->startx - 1)
             / (state->max - state->min) + state->startx;
    if (redraw || newx < state->currentx) progressbar_drawborderbg(state, newx);
    if (newx > state->currentx)
        displaylcd(state->currentx + 1, newx, state->starty + 1, state->endy - 1,
                   (void*)-1, state->fgcolor);
    state->currentx = newx;
}
