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
#include "usb/usbtarget.h"
#include "usb/usbdrv.h"
#include "storage_ata-target.h"


int usb_target_handle_request(uint32_t* buffer, int bufsize)
{
    int size = 0;
    switch (buffer[0])
    {
        case 0xffff0001:  // GET DISK INFO
        {
            buffer[0] = 1;
            buffer[1] = (uint32_t)ata_identify_data;
            buffer[2] = ata_total_sectors;
            buffer[3] = ata_total_sectors >> 32;
#ifdef ATA_HAVE_BBT
            buffer[4] = ata_virtual_sectors;
            buffer[5] = ata_virtual_sectors >> 32;
            buffer[6] = (uint32_t)ata_bbt;
            buffer[7] = ATA_BBT_PAGES << 6;
#else
            buffer[4] = ata_total_sectors;
            buffer[5] = ata_total_sectors >> 32;
            buffer[6] = 0;
            buffer[7] = 0;
#endif
            size = 32;
            break;
        }
        case 0xffff0002:  // LOWLEVEL DISK ACCESS
        {
#ifdef ATA_HAVE_BBT
            int rc = ata_rw_sectors_internal((((uint64_t)(buffer[3])) << 32) | buffer[2],
                                             buffer[4], (void*)(buffer[5]), (bool)(buffer[1]));
#else
            int rc = ata_rw_sectors((((uint64_t)(buffer[3])) << 32) | buffer[2],
                                    buffer[4], (void*)(buffer[5]), (bool)(buffer[1]));
#endif
            buffer[0] = 1;
            buffer[1] = (uint32_t)rc;
            size = 16;
            break;
        }
        default:
            buffer[0] = 2;
            size = 16;
    }
    return size;
}