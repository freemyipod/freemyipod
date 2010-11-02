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
#include "console.h"
#include "execimage.h"
#include "mmu.h"


int execimage(void* image)
{
    struct execimage_header* header = (struct execimage_header*)image;
    if (memcmp(header, "emBIexec", 8))
    {
        cprintf(CONSOLE_BOOT, "execimage: Bad signature!\n"
                              "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                header->signature[0], header->signature[1], header->signature[2],
                header->signature[3], header->signature[4], header->signature[5],
                header->signature[6], header->signature[7]);
        return -1;
    }
    if (header->version > 0)
    {
        cprintf(CONSOLE_BOOT, "execimage: Unsupported version! (%08X)\n", header->version);
        return -2;
    }
    if (header->baseaddr != image)
    {
        cprintf(CONSOLE_BOOT, "execimage: Image loaded to wrong address! "
                              "(expected: %08X, got: %08X)\n", header->baseaddr, image);
        return -3;
    }
    clean_dcache();
    invalidate_icache();
    return thread_create(header->threadname, header->entrypoint, header->stackaddr,
                         header->stacksize, header->threadtype, header->threadpriority, true);
}
