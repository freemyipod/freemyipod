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
#include "bootflash.h"
#include "contextswitch.h"
#include "util.h"


#define nor ((uint8_t*)0x24000000)
#define norword ((uint32_t*)0x24000000)
#define norflsh ((volatile uint16_t*)0x24000000)


extern void control_nor_cache(bool enable);


static void* findflashfile(const char* filename, uint32_t* size)
{
    uint32_t i;
    for (i = 0; i < 0x1000; i += 0x10)
        if (memcmp(&nor[i], filename, 8) == 0)
        {
            *size = norword[(i + 0xc) >> 2];
            return &nor[norword[(i + 0x8) >> 2]];
        }
    return 0;
}

int bootflash_filesize(const char* filename)
{
    uint32_t size;
    if (findflashfile(filename, &size)) return size & 0xfffff;
    else return -1;
}

int bootflash_attributes(const char* filename)
{
    uint32_t size;
    if (findflashfile(filename, &size)) return size >> 20;
    else return -1;
}

void* bootflash_getaddr(const char* filename)
{
    uint32_t size;
    return findflashfile(filename, &size);
}

int bootflash_read(const char* filename, void* addr, int offset, int size)
{
    uint32_t fsize;
    uint8_t* file = (uint8_t*)findflashfile(filename, &fsize);
    if (!file) return -1;
    fsize &= 0xfffff;
    if (offset + size > fsize) size = fsize - offset;
    if (size > 0) memcpy(addr, &file[offset], size);
    return size;
}

void bootflash_readraw(void* addr, int offset, int size)
{
    memcpy(addr, &nor[offset], size);
}

void bootflash_writeraw(void* addr, int offset, int size)
{
    uint32_t mode = enter_critical_section();
    control_nor_cache(false);

    while (size > 0)
    {
        int remainder = MIN(0x1000 - (offset & 0xfff), size);
        if (memcmp(&nor[offset], addr, remainder))
        {
            bool needserase = false;
            int i;
            for (i = 0; i < remainder; i += 2)
                if (norflsh[(offset + i) >> 1] != 0xffff)
                    needserase = true;
            if (needserase)
            {
                norflsh[0x5555] = 0xaa;
                norflsh[0x2aaa] = 0x55;
                norflsh[0x5555] = 0x80;
                norflsh[0x5555] = 0xaa;
                norflsh[0x2aaa] = 0x55;
                norflsh[(offset & ~0xfff) >> 1] = 0x30;
                while (norflsh[(offset & ~0xfff) >> 1] != 0xffff);
            }
            for (i = 0; i < remainder; i += 2)
                if (norflsh[(offset + i) >> 1] != ((uint16_t*)addr)[i >> 1])
                {
                    norflsh[0x5555] = 0xaa;
                    norflsh[0x2aaa] = 0x55;
                    norflsh[0x5555] = 0xa0;
                    norflsh[(offset + i) >> 1] = ((uint16_t*)addr)[i >> 1];
                    while (norflsh[(offset + i) >> 1] != ((uint16_t*)addr)[i >> 1]);
                }
        }
        addr = (void*)(((uint32_t)addr) + remainder);
        offset += remainder;
        size -= remainder;
    }

    control_nor_cache(true);
    leave_critical_section(mode);
}

void* bootflash_getrawaddr(int offset)
{
    return &nor[offset];
}

bool bootflash_is_memmapped()
{
#ifdef BOOTFLASH_IS_MEMMAPPED
    return true;
#else
    return false;
#endif
}
