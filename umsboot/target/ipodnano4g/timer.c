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
#include "s5l8720.h"


void setup_tick()
{
    int cycles = SYSTEM_TICK / 100;
    
    TACMD = (1 << 1);   /* TA_CLR */
    TBCMD = (1 << 1);   /* TB_CLR */
    TCCMD = (1 << 1);   /* TC_CLR */
    TDCMD = (1 << 1);   /* TD_CLR */
    TECMD = (1 << 1);   /* TE_CLR */
    TFCMD = (1 << 1);   /* TF_CLR */
    TGCMD = (1 << 1);   /* TG_CLR */
    THCMD = (1 << 1);   /* TH_CLR */

    /* configure timer for 10 kHz */
    TBPRE = 208 - 1;    /* prescaler */
    TBCON = (0 << 13) | /* TB_INT1_EN */
            (1 << 12) | /* TB_INT0_EN */
            (0 << 11) | /* TB_START */
            (3 << 8) |  /* TB_CS = PCLK / 64 */
            (0 << 4);   /* TB_MODE_SEL = interval mode */
    TBDATA0 = cycles;   /* set interval period */
    TBCMD = (1 << 0);   /* TB_EN */
}

void INT_TIMERB(void)
{
    TBCON = TBCON;
    scheduler_switch(-1);
}
