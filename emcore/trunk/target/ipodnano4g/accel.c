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
#include "i2c.h"
#include "accel.h"

void accel_read_force_vector(struct accel_vector* values)
{
    int8_t buf[5];

    i2c_recv(0, 0x3a, 0x29 | 0x80, buf, 5);

    values->x = ((int32_t)buf[0]) * 1207960;
    values->y = ((int32_t)buf[2]) * 1207960;
    values->z = ((int32_t)buf[4]) * 1207960;
}
