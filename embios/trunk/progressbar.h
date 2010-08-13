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


#ifndef __PROGRESSBAR_H__
#define __PROGRESSBAR_H__


#include "global.h"
#include "lcd.h"


struct progressbar_state
{
    int startx;
    int endx;
    int starty;
    int endy;
    int bordercolor;
    int bgcolor;
    int fgcolor;
    int currentx;
    int min;
    int max;
};


void progressbar_init(struct progressbar_state* state, int startx, int endx, int starty, int endy,
                      int bordercolor, int bgcolor, int fgcolor, int min, int max);
void progressbar_setpos(struct progressbar_state* state, int value, bool redraw);


#endif
