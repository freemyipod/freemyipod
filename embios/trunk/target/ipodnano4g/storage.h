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


#ifndef __FTL_H__
#define __FTL_H__

#include "global.h"

uint32_t storage_init();
uint32_t storage_read(uint32_t sector, uint32_t count, void* buffer);
uint32_t storage_write(uint32_t sector, uint32_t count, const void* buffer);
uint32_t storage_sync();
uint32_t storage_get_sector_count();


#endif
