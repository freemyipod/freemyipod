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

//    iPod nano 5G accelerometer (diag_TestAccelerometer:0xb7c): 7-bit slave 0x19 (8-bit 0x32),
//    WHO_AM_I (0x0f) = 0x12, power-up CTRL_REG1 0x20 = 0x27, data OUT_X..Z from 0x28 | 0x80 (auto-increment).
//    PROVISIONAL: bus 0 is inferred (EFI SMBus handle); the 4G used 7-bit 0x1d / reg 0x29. Power-up happens lazily on
//    the first read (thread context), so a plain boot never talks to the device.

#include "global.h"
#include "i2c.h"
#include "accel.h"

#define ACCEL_BUS 0
#define ACCEL_ADDR 0x32

static bool accel_powered;
uint8_t accel_whoami;   /* host-readable: last WHO_AM_I read (expect 0x12) */

void accel_read_force_vector(struct accel_vector* values)
{
    int8_t buf[6];

    if (!accel_powered)
    {
        accel_whoami = i2c_recvbyte(ACCEL_BUS, ACCEL_ADDR, 0x0f);
        i2c_sendbyte(ACCEL_BUS, ACCEL_ADDR, 0x20, 0x27);   /* diag: CTRL_REG1 = 0x27 (power on, X/Y/Z enabled) */
        accel_powered = true;
    }
    i2c_recv(ACCEL_BUS, ACCEL_ADDR, 0x28 | 0x80, (uint8_t*)buf, 6);

    /* high bytes = signed 8-bit samples (left-justified devices); same scale as the 4G driver */
    values->x = ((int32_t)buf[1]) * 1207960;
    values->y = ((int32_t)buf[3]) * 1207960;
    values->z = ((int32_t)buf[5]) * 1207960;
}
