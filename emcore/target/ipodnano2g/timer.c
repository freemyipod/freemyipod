//
//
//    Copyright 2010 TheSeven
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


#include "global.h"
#include "timer.h"
#include "thread.h"
#include "s5l8701.h"


void timer_init()
{
}

void timer_schedule_wakeup(uint32_t usecs)
{
    if (usecs > 11184640)
    {
        TBPRE = 511;
        TBDATA1 = 65535;
    }
    else if (usecs > 21845)
    {
        TBPRE = 511;
        TBDATA1 = (usecs * 192) >> 15;
    }
    else
    {
        TBPRE = 0;
        TBDATA1 = (usecs * 192) >> 6;
    }
    TBCON = (1 << 13) | /* TB_INT1_EN */
            (0 << 12) | /* TB_INT0_EN */
            (0 << 11) | /* TB_START */
            (2 << 8) |  /* TB_CS = PCLK / 16 */
            (2 << 4);   /* TB_MODE_SEL = one-shot mode */
    TBCMD = (1 << 1);   /* TB_CLR */
    TBCMD = (1 << 0);   /* TB_EN */
}

void timer_kill_wakeup()
{
    TBCMD = (1 << 1);   /* TB_CLR */
    TBCON = TBCON;
}

void INT_TIMERB(void)
{
    TBCON = TBCON;
    scheduler_switch(NULL, NULL);
}
