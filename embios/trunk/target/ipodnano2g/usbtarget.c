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


#include "global.h"
#include "usb/usbtarget.h"
#include "usb/usbdrv.h"
#include "nand.h"


int usb_target_handle_request(uint32_t* buffer, int bufsize)
{
    int size = 0;
    switch (buffer[0])
    {
        case 0xffff0001:  // GET NAND INFO
        {
            int banks = 1;
            const struct nand_device_info_type* type = nand_get_device_type(0);
            for (; banks < 4; banks++)
                if (!nand_get_device_type(banks))
                    break;
            buffer[0] = 1;
            buffer[1] = type->id;
            buffer[2] = (banks << 16) | type->pagesperblock;
            buffer[3] = (type->blocks << 16) | type->userblocks;
            size = 16;
            break;
        }
        case 0xffff0002:  // READ NAND PAGES
        case 0xffff0003:  // WRITE NAND PAGES
        {
            int banks = 1;
            const struct nand_device_info_type* type = nand_get_device_type(0);
            for (; banks < 4; banks++)
                if (!nand_get_device_type(banks))
                    break;
            uint32_t pagecount = banks * type->blocks * type->pagesperblock;
            if (buffer[2] + buffer[3] > pagecount)
            {
                buffer[0] = 0xffff0001;
                size = 16;
                break;
            }
            int i;
            uint32_t database = buffer[1] & ~0xc0000000;
            uint32_t sparebase = database + buffer[3] * 2048;
            uint32_t resultbase = sparebase + buffer[3] * 64;
            int doecc = buffer[1] & 0x80000000;
            int checkempty = buffer[1] & 0x40000000;
            for (i = 0; i < buffer[3]; i++)
            {
                int lpage = buffer[2] + i;
                int bank = lpage % banks;
                int page = lpage / banks;
                int result;
                if (buffer[0] == 0xffff0002)
                    result = nand_read_page(bank, page, (void*)(database + i * 2048),
                                            (void*)(sparebase + i * 64), doecc, checkempty);
                else if (buffer[0] == 0xffff0003)
                    result = nand_write_page(bank, page, (void*)(database + i * 2048),
                                             (void*)(sparebase + i * 64), doecc);
                *((int*)(resultbase + i * 4)) = result;
            }
            buffer[0] = 1;
            size = 16;
            break;
        }
        case 0xffff0004:  // ERASE NAND PAGES
        {
            int banks = 1;
            const struct nand_device_info_type* type = nand_get_device_type(0);
            for (; banks < 4; banks++)
                if (!nand_get_device_type(banks))
                    break;
            uint32_t blockcount = banks * type->blocks;
            if (buffer[2] + buffer[3] > blockcount)
            {
                buffer[0] = 0xffff0001;
                size = 16;
                break;
            }
            int i;
            for (i = 0; i < buffer[3]; i++)
            {
                int lblock = buffer[2] + i;
                int bank = lblock % banks;
                int block = lblock / banks;
                int result = nand_block_erase(bank, block * type->pagesperblock); 
                *((int*)(buffer[1] + i * 4)) = result;
            }
            buffer[0] = 1;
            size = 16;
            break;
        }
        default:
            buffer[0] = 2;
            size = 16;
    }
    return size;
}