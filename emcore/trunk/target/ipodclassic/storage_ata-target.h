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


#ifndef __STORAGE_ATA_TARGET_H__
#define __STORAGE_ATA_TARGET_H__

#include "global.h"


extern uint16_t ata_identify_data[0x100];
extern uint64_t ata_total_sectors;

#ifdef ATA_HAVE_BBT
extern uint16_t ata_bbt[ATA_BBT_PAGES][0x20];
extern uint64_t ata_virtual_sectors;

int ata_rw_sectors_internal(uint64_t sector, uint32_t count, void* buffer, bool write);
#endif


#endif
