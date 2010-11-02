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
#include "timer.h"
#include "thread.h"
#include "s5l8701.h"


void setup_tick()
{
    int cycles = SYSTEM_TICK / 100;
    
    /* configure timer for 10 kHz */
    TBCMD = (1 << 1);   /* TB_CLR */
    TBPRE = 300 - 1;    /* prescaler */
    TBCON = (0 << 13) | /* TB_INT1_EN */
            (1 << 12) | /* TB_INT0_EN */
            (0 << 11) | /* TB_START */
            (2 << 8) |  /* TB_CS = PCLK / 16 */
            (0 << 4);   /* TB_MODE_SEL = interval mode */
    TBDATA0 = cycles;   /* set interval period */
    TBCMD = (1 << 0);   /* TB_EN */
}

void INT_TIMERB(void)
{
    TBCON = TBCON;
    scheduler_switch(-1);
}
