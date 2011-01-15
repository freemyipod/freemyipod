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
#include "bootflash.h"
#include "contextswitch.h"
#include "util.h"
#include "spi.h"
#include "s5l8702.h"


void bootflash_ce(int port, bool state)
{
    if (state)
    {
        if (port == 2) PDAT(0xe) &= ~0x40;
        else if (port == 1) PDAT(6) &= ~0x10;
        else PDAT(0) &= ~1;
    }
    else
    {
        if (port == 2) PDAT(0xe) |= 0x40;
        else if (port == 1) PDAT(6) |= 0x10;
        else PDAT(0) |= 1;
    }
}

void bootflash_wait_ready(int port)
{
    while (true)
    {
        bootflash_ce(port, true);
        spi_write(port, 5);
        if (!(spi_write(port, 0xff) & 1)) break;
        bootflash_ce(port, false);
    }
    bootflash_ce(port, false);
}

void bootflash_enable_writing(int port, bool state)
{
    if (!state)
    {
        bootflash_ce(port, true);
        spi_write(port, 4);
        bootflash_ce(port, false);
    }
    bootflash_ce(port, true);
    spi_write(port, 0x50);
    bootflash_ce(port, false);
    bootflash_ce(port, true);
    spi_write(port, 1);
    spi_write(port, state ? 0 : 0x1c);
    bootflash_ce(port, false);
    if (state)
    {
        bootflash_ce(port, true);
        spi_write(port, 6);
        bootflash_ce(port, false);
    }
}

void bootflash_readraw(void* addr, int offset, int size)
{
    spi_prepare(0);
    bootflash_wait_ready(0);
    bootflash_ce(0, true);
    spi_write(0, 3);
    spi_write(0, (offset >> 16) & 0xff);
    spi_write(0, (offset >> 8) & 0xff);
    spi_write(0, offset & 0xff);
    spi_read(0, size, addr);
    bootflash_ce(0, false);
    spi_release(0);
}

void bootflash_write_internal(int port, uint32_t addr, uint32_t size, void* buf)
{
    uint8_t* buffer = (uint8_t*)buf;
    bool first = true;
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_enable_writing(port, true);
    while (size)
    {
        bootflash_ce(port, true);
        spi_write(port, 0xad);
        if (first)
        {
            spi_write(port, (addr >> 16) & 0xff);
            spi_write(port, (addr >> 8) & 0xff);
            spi_write(port, addr & 0xff);
            first = false;
        }
        spi_write(port, *buffer++);
        spi_write(port, *buffer++);
        bootflash_ce(port, false);
        bootflash_wait_ready(port);
        size -= 2;
    }
    bootflash_enable_writing(port, false);
    spi_release(port);
}

void bootflash_erase_internal(int port, uint32_t addr)
{
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_enable_writing(port, true);
    bootflash_ce(port, true);
    spi_write(port, 0x20);
    spi_write(port, (addr >> 16) & 0xff);
    spi_write(port, (addr >> 8) & 0xff);
    spi_write(port, addr & 0xff);
    bootflash_ce(port, false);
    bootflash_enable_writing(port, false);
    spi_release(port);
}

int bootflash_compare(int offset, void* addr, int size)
{
    int i;
    int result = 0;
    uint8_t buf[32];
    spi_prepare(0);
    bootflash_wait_ready(0);
    bootflash_ce(0, true);
    spi_write(0, 3);
    spi_write(0, (offset >> 16) & 0xff);
    spi_write(0, (offset >> 8) & 0xff);
    spi_write(0, offset & 0xff);
    while (size > 0)
    {
        spi_read(0, MIN(sizeof(buf), size), buf);
        if (memcmp((uint8_t*)addr, buf, MIN(sizeof(buf), size))) result |= 1;
        for (i = 0; i < MIN(sizeof(buf), size); i += 2)
            if (buf[i] != 0xff) result |= 2;
        addr = (void*)(((uint32_t)addr) + sizeof(buf));
        size -= sizeof(buf);
    }
    bootflash_ce(0, false);
    spi_release(0);
    return result;
}

void bootflash_writeraw(void* addr, int offset, int size)
{
    int i;
    bool needswrite;
    while (size > 0)
    {
        int remainder = MIN(0x1000 - (offset & 0xfff), size);
        int contentinfo = bootflash_compare(offset, addr, remainder);
        if (contentinfo & 1)
        {
            if (contentinfo & 2) bootflash_erase_internal(0, offset & ~0xfff); 
            needswrite = false;
            for (i = 0; i < remainder; i += 1)
                if (((uint8_t*)addr)[i] != 0xff)
                {
                    needswrite = true;
                    break;
                }
            if (needswrite) bootflash_write_internal(0, offset, remainder, addr);
        }
        addr = (void*)(((uint32_t)addr) + remainder);
        offset += remainder;
        size -= remainder;
    }
}

static uint32_t findflashfile(const char* filename, uint32_t* size)
{
    uint32_t i;
    uint32_t buf[4];
    spi_prepare(0);
    bootflash_wait_ready(0);
    bootflash_ce(0, true);
    spi_write(0, 3);
    spi_write(0, 0);
    spi_write(0, 0);
    spi_write(0, 0);
    for (i = 0; i < 0x100; i++)
    {
        spi_read(0, 16, buf);
        if (!memcmp(buf, filename, 8))
        {
            *size = buf[3];
            bootflash_ce(0, false);
            spi_release(0);
            return buf[2];
        }
    }
    bootflash_ce(0, false);
    spi_release(0);
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
    return NULL;
}

int bootflash_read(const char* filename, void* addr, int offset, int size)
{
    uint32_t fsize;
    uint32_t file = findflashfile(filename, &fsize);
    if (!file) return -1;
    fsize &= 0xfffff;
    if (offset + size > fsize) size = fsize - offset;
    if (size > 0) bootflash_readraw(addr, file + offset, size);
    return size;
}

void* bootflash_getrawaddr(int offset)
{
    return NULL;
}

bool bootflash_is_memmapped()
{
    return false;
}
