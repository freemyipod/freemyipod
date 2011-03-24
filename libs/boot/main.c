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


#include "emcorelib.h"
#include "export/libboot.h"


static struct libboot_api apitable =
{
    .verify_rockbox_checksum = verify_rockbox_checksum
};


EMCORE_LIB_HEADER(LIBBOOT_IDENTIFIER, LIBBOOT_API_VERSION, LIBBOOT_MIN_API_VERSION,
                  NULL, NULL, apitable)


int verify_rockbox_checksum(void* image, size_t size)
{
    int i;
    uint8_t* data = (uint8_t*)image;
    uint32_t checksum = data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
    uint32_t platform;
    switch (get_platform_id())
    {
        case 0x47324e49:  // IN2G
            checksum -= 62;
            platform = 0x67326e6e;  // nn2g
            break;
        case 0x4c435049:  // IPCL
            checksum -= 71;
            platform = 0x67367069;  // ip6g
            break;
        default:
            return -3;
    }
    if (*((uint32_t*)&data[4]) != platform) return -2;
    for (i = 0; i < size - 8; i++)
    {
        data[i] = data[i + 8];
        checksum -= data[i + 8];
    }
    if (checksum) return -1;
    return 0;
}
