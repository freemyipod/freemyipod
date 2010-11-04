//
//
//    Copyright 2009 TheSeven
//
//
//    This file is part of the Linux4Nano toolkit.
//
//    TheSeven's iBugger is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    TheSeven's iBugger is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with the Linux4Nano toolkit.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __NAND_H__
#define __NAND_H__

#include <toolkit.h>


#define NAND_CMD_READ       0x00
#define NAND_CMD_PROGCNFRM  0x10
#define NAND_CMD_READ2      0x30
#define NAND_CMD_BLOCKERASE 0x60
#define NAND_CMD_GET_STATUS 0x70
#define NAND_CMD_PROGRAM    0x80
#define NAND_CMD_ERASECNFRM 0xD0
#define NAND_CMD_RESET      0xFF

#define NAND_STATUS_READY   0x40


struct nand_device_info_type
{
    uint32_t id;
    uint16_t blocks;
    uint16_t userblocks;
    uint8_t blocksizeexponent;
    uint8_t tunk1;
    uint8_t twp;
    uint8_t dummy;
} __attribute__((packed));

uint32_t nand_read_page(uint32_t bank, uint32_t page, void* data, void* spare, uint32_t checkempty);
const struct nand_device_info_type* nand_get_device_type(uint32_t bank);
uint32_t nand_reset(uint32_t bank);
uint32_t nand_init();


#endif
