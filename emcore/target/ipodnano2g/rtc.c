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


#include "global.h"
#include "pmu.h"
#include "rtc.h"


void rtc_read_datetime(struct rtc_datetime* dt)
{
    pmu_read_rtc((unsigned char *)dt);

    dt->second = BCD2DEC(dt->second);
    dt->minute = BCD2DEC(dt->minute);
    dt->hour = BCD2DEC(dt->hour);
    dt->day = BCD2DEC(dt->day);
    dt->month = BCD2DEC(dt->month);
    dt->year = BCD2DEC(dt->year);
}

void rtc_write_datetime(const struct rtc_datetime* dt)
{
    unsigned char buf[7];

    buf[0] = DEC2BCD(dt->second);
    buf[1] = DEC2BCD(dt->minute);
    buf[2] = DEC2BCD(dt->hour);
    buf[3] = dt->weekday;
    buf[4] = DEC2BCD(dt->day);
    buf[5] = DEC2BCD(dt->month);
    buf[6] = DEC2BCD(dt->year);

    pmu_write_rtc(buf);
}
