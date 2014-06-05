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
#include "usb/usb.h"
#include "storage_ata-target.h"
#include "thread.h"


int usb_target_handle_request(uint32_t* buf, int bufsize, void** addr)
{
    int len = 0;
    switch (buf[0])
    {
        case 0xffff0001:  // GET DISK INFO
            buf[0] = 1;
            buf[1] = (uint32_t)ata_identify_data;
            buf[2] = ata_total_sectors;
            buf[3] = ata_total_sectors >> 32;
#ifdef ATA_HAVE_BBT
            buf[4] = ata_virtual_sectors;
            buf[5] = ata_virtual_sectors >> 32;
            buf[6] = (uint32_t)ata_bbt;
            buf[7] = 0;
#else
            buf[4] = ata_total_sectors;
            buf[5] = ata_total_sectors >> 32;
            buf[6] = 0;
            buf[7] = 0;
#endif
            len = 16;
            break;
        case 0xffff0002:  // LOWLEVEL DISK ACCESS
            buf[0] = 1;
            mutex_lock(&ata_mutex, TIMEOUT_BLOCK);
#ifdef ATA_HAVE_BBT
            buf[1] = (uint32_t)ata_rw_sectors_internal((((uint64_t)(buf[3])) << 32) | buf[2],
                                                       buf[4], (void*)(buf[5]), (bool)(buf[1]));
#else
            buf[1] = (uint32_t)ata_rw_sectors((((uint64_t)(buf[3])) << 32) | buf[2],
                                              buf[4], (void*)(buf[5]), (bool)(buf[1]));
#endif
            mutex_unlock(&ata_mutex);
            break;
        case 0xffff0003:  // ATA_BBT_RELOAD
            ata_bbt_reload();
            buf[0] = 1;
            break;
        case 0xffff0004:  // ATA_BBT_DISABLE
            ata_bbt_disable();
            buf[0] = 1;
            break;
        default:
            buf[0] = 2;
            break;
    }
    return len;
}
