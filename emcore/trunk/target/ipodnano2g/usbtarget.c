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
#include "nand.h"


int usb_target_handle_request(uint32_t* buf, int bufsize, void** addr)
{
    int len = 0;
    switch (buf[0])
    {
        case 0xffff0001:  // GET NAND INFO
        {
            int banks;
            const struct nand_device_info_type* type = nand_get_device_type(0);
            for (banks = 1; banks < 4; banks++)
                if (!nand_get_device_type(banks))
                    break;
            buf[0] = 1;
            buf[1] = type->id;
            buf[2] = (banks << 16) | type->pagesperblock;
            buf[3] = (type->blocks << 16) | type->userblocks;
            break;
        }
        case 0xffff0002:  // READ NAND PAGES
        case 0xffff0003:  // WRITE NAND PAGES
        {
            int banks;
            const struct nand_device_info_type* type = nand_get_device_type(0);
            for (banks = 1; banks < 4; banks++)
                if (!nand_get_device_type(banks))
                    break;
            uint32_t pagecount = banks * type->blocks * type->pagesperblock;
            if (buf[2] + buf[3] > pagecount)
            {
                buf[0] = 0xffff0001;
                break;
            }
            int i;
            uint32_t database = buf[1] & ~0xc0000000;
            uint32_t sparebase = database + buf[3] * 2048;
            uint32_t resultbase = sparebase + buf[3] * 64;
            int doecc = buf[1] & 0x80000000;
            int checkempty = buf[1] & 0x40000000;
            for (i = 0; i < buf[3]; i++)
            {
                int lpage = buf[2] + i;
                int bank = lpage % banks;
                int page = lpage / banks;
                int result;
                if (buf[0] == 0xffff0002)
                    result = nand_read_page(bank, page, (void*)(database + i * 2048),
                                            (void*)(sparebase + i * 64), doecc, checkempty);
                else if (buf[0] == 0xffff0003)
                    result = nand_write_page(bank, page, (void*)(database + i * 2048),
                                             (void*)(sparebase + i * 64), doecc);
                *((int*)(resultbase + i * 4)) = result;
            }
            buf[0] = 1;
            break;
        }
        case 0xffff0004:  // ERASE NAND PAGES
        {
            int banks;
            const struct nand_device_info_type* type = nand_get_device_type(0);
            for (banks = 1; banks < 4; banks++)
                if (!nand_get_device_type(banks))
                    break;
            uint32_t blockcount = banks * type->blocks;
            if (buf[2] + buf[3] > blockcount)
            {
                buf[0] = 0xffff0001;
                break;
            }
            int i;
            for (i = 0; i < buf[3]; i++)
            {
                int lblock = buf[2] + i;
                int bank = lblock % banks;
                int block = lblock / banks;
                int result = nand_block_erase(bank, block * type->pagesperblock); 
                *((int*)(buf[1] + i * 4)) = result;
            }
            buf[0] = 1;
            break;
        }
        default:
            buf[0] = 2;
            break;
    }
    return len;
}
