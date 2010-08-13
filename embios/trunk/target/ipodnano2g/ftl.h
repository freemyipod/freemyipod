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


extern const struct nand_device_info_type* ftl_nand_type;
extern uint32_t ftl_banks;


uint32_t ftl_init(void) INITCODE_ATTR;
uint32_t ftl_read(uint32_t sector, uint32_t count, void* buffer);
uint32_t ftl_write(uint32_t sector, uint32_t count, const void* buffer);
uint32_t ftl_sync(void);


#endif
