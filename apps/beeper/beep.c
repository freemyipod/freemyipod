//
//
//    Copyright 2011 TheSeven, user890104
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
#include "beep.h"

void singlebeep(unsigned int cycles, unsigned int time)
{
    /* configure timer for 100 kHz */
    TDCMD = (1 << 1);   /* TD_CLR */
    TDPRE = 30 - 1;    /* prescaler */
    TDCON =
            //(1 << 13) | /* TD_INT1_EN */
            //(0 << 12) | /* TD_INT0_EN */
            //(0 << 11) | /* TD_START */
            (2 << 8) |  /* TD_CS = PCLK / 16 */
            (1 << 4);   /* TD_MODE_SEL = PWM mode */
    TDDATA0 = cycles;   /* set interval period */
    TDDATA1 = cycles << 1; /* set interval period */
    TDCMD = (1 << 0);   /* TD_EN */
    sleep(time);
    TDCMD = (1 << 1);   /* TD_CLR */
}
