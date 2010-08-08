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


#ifndef __NAND_H__
#define __NAND_H__

#include "global.h"


struct nand_device_info_type
{
    uint32_t id;
    uint16_t blocks;
    uint16_t userblocks;
    uint16_t pagesperblock;
    uint8_t blocksizeexponent;
    uint8_t tunk1;
    uint8_t twp;
    uint8_t tunk2;
    uint8_t tunk3;
} __attribute__((packed));

uint32_t nand_read_page(uint32_t bank, uint32_t page, void* databuffer,
                        void* sparebuffer, uint32_t doecc,
                        uint32_t checkempty);
uint32_t nand_write_page(uint32_t bank, uint32_t page, void* databuffer,
                         void* sparebuffer, uint32_t doecc);
uint32_t nand_block_erase(uint32_t bank, uint32_t page);

uint32_t nand_read_page_fast(uint32_t page, void* databuffer,
                             void* sparebuffer, uint32_t doecc,
                             uint32_t checkempty);
uint32_t nand_write_page_start(uint32_t bank, uint32_t page, void* databuffer,
                               void* sparebuffer, uint32_t doecc);
uint32_t nand_write_page_collect(uint32_t bank);

const struct nand_device_info_type* nand_get_device_type(uint32_t bank);
uint32_t nand_reset(uint32_t bank);
int nand_device_init(void);
void nand_set_active(void);
long nand_last_activity(void);
void nand_power_up(void);
void nand_power_down(void);


#endif
