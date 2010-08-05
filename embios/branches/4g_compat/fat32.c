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
#include "storage.h"
#include "fat32.h"
#include "util.h"


uint32_t fat32_ok;
uint32_t fat32_startsector;
uint32_t fat32_secperclus;
uint32_t fat32_database;
uint32_t fat32_fatbase;
uint32_t fat32_fatsize;
uint32_t fat32_fatcount;
uint32_t fat32_sectorcount;
uint32_t fat32_clustercount;
uint32_t fat32_rootdirclus;
uint32_t fat32_buf1[0x200] __attribute__((aligned(16)));
uint32_t fat32_buf2[0x200] __attribute__((aligned(16)));


uint32_t fat32_get_root()
{
    return fat32_rootdirclus;
}

uint32_t fat32_get_clusterchain(uint32_t clusterchain, uint32_t maxsize, void* buffer)
{
    uint32_t i;
    for (i = 0; i < (maxsize >> 11); )
    {
        uint32_t sector = (clusterchain - 2) * fat32_secperclus + fat32_database;
        uint32_t count = fat32_secperclus;
        if (count + i > (maxsize >> 11)) count = (maxsize >> 11) - i;
        uint32_t fatsector = fat32_fatbase + (clusterchain >> 9);
        if (storage_read(fatsector, 1, &((uint32_t*)buffer)[i << 9])) return 1;
        clusterchain = ((uint32_t*)buffer)[(i << 9) + (clusterchain & 0x1FF)];
        if (storage_read(sector, count, &((uint32_t*)buffer)[i << 9])) return 1;
        i += count;
        if (clusterchain >= 0x0ffffff0) return 0;
    }
    return clusterchain;
}

uint32_t fat32_get_direntry(uint32_t clusterchain, const char* filename, uint32_t* filesize)
{
    uint32_t i, j;
    while (clusterchain > 1 && clusterchain < 0x0ffffff0)
    {
        uint32_t sector = (clusterchain - 2) * fat32_secperclus + fat32_database;
        for (j = 0; j < fat32_secperclus; j++)
        {
            if (storage_read(sector + j, 1, fat32_buf1)) return 1;
            for (i = 0; i < 0x200; i += 8)
                if (((uint8_t*)fat32_buf1)[i << 2] == 0) return 0;
                else if (((uint8_t*)fat32_buf1)[i << 2] == 0xe5) continue;
                else if (memcmp(&fat32_buf1[i], filename, 11) == 0)
                {
                    *filesize = fat32_buf1[i + 7];
                    return (((uint16_t*)fat32_buf1)[(i << 1) + 0xA] << 16)
                         | ((uint16_t*)fat32_buf1)[(i << 1) + 0xD];
                }
        }
        uint32_t fatsector = fat32_fatbase + (clusterchain >> 9);
        if (storage_read(fatsector, 1, fat32_buf1)) return 1;
        clusterchain = fat32_buf1[(i << 9) + (clusterchain & 0x1FF)];
    }
    return 0;
}

uint32_t fat32_delete_clusterchain(uint32_t clusterchain)
{
    while (1)
    {
        uint32_t fatsector = fat32_fatbase + (clusterchain >> 9);
        if (storage_read(fatsector, 1, fat32_buf1)) return 1;
        clusterchain = fat32_buf1[clusterchain & 0x1FF];
        fat32_buf1[(clusterchain & 0x1FF)] = 0;
        if (storage_write(fatsector, 1, fat32_buf1)) return 1;
        if (clusterchain >= 0x0ffffff0) return 0;
    }
}

uint32_t fat32_delete_direntry(uint32_t clusterchain, const char* filename)
{
    uint32_t i, j;
    while (clusterchain > 1 && clusterchain < 0x0ffffff0)
    {
        uint32_t sector = (clusterchain - 2) * fat32_secperclus + fat32_database;
        for (j = 0; j < fat32_secperclus; j++)
        {
            if (storage_read(sector + j, 1, fat32_buf1)) return 1;
            for (i = 0; i < 0x200; i += 8)
                if (((uint8_t*)fat32_buf1)[i << 2] == 0) return 0;
                else if (((uint8_t*)fat32_buf1)[i << 2] == 0xe5) continue;
                else if (memcmp(&fat32_buf1[i], filename, 11) == 0)
                {
                    ((uint8_t*)fat32_buf1)[i << 2] = 0xe5;
                    if (storage_write(sector + j, 1, fat32_buf1)) return 1;
                    return 0;
                }
        }
        uint32_t fatsector = fat32_fatbase + (clusterchain >> 9);
        if (storage_read(fatsector, 1, fat32_buf1)) return 1;
        clusterchain = fat32_buf1[(i << 9) + (clusterchain & 0x1FF)];
    }
    return 0;
}

uint32_t fat32_store_stream(void* buffer, uint32_t size)
{
    uint32_t i;
    uint32_t clusterchain = 0;
    uint32_t scanidx = 2;
    uint32_t scansect = 0xffffffff;
    uint32_t lastidx;
    uint32_t lastsect = 0xffffffff;
    uint32_t dirty = 0;
    while (size)
    {
        while (scanidx < fat32_clustercount + 2)
        {
            if ((scanidx >> 9) != scansect)
            {
                scansect = scanidx >> 9;
                if (storage_read(fat32_fatbase + scansect, 1, fat32_buf1)) return 0;
            }
            if (!fat32_buf1[scanidx & 0x1ff]) break;
            scanidx++;
        }
        if (scanidx >= fat32_clustercount + 2) return 0;
        if (!clusterchain) clusterchain = scanidx;
        else
        {
            fat32_buf2[lastidx & 0x1ff] = scanidx;
            dirty = 1;
        }
        lastidx = scanidx;
        if ((lastidx >> 9) != lastsect)
        {
            if (dirty)
                if (storage_write(fat32_fatbase + lastsect, 1, fat32_buf2)) return 0;
            dirty = 0;
            lastsect = lastidx >> 9;
            memcpy(fat32_buf2, fat32_buf1, 0x800);
        }
        uint32_t sector = (scanidx - 2) * fat32_secperclus + fat32_database;
        uint32_t count = (size + 0x7ff) >> 11;
        if (count > fat32_secperclus) count = fat32_secperclus;
        if (storage_write(sector, count, &((uint32_t*)buffer)[i << 9])) return 0;
        if ((count << 11) >= size)
        {
            fat32_buf2[lastidx & 0x1ff] = 0x0fffffff;
            if (storage_write(fat32_fatbase + lastsect, 1, fat32_buf2)) return 0;
            break;
        }
        size -= count << 11;
        buffer = (void*)((uint32_t)buffer + (count << 11));
        scanidx++;
    }
    return clusterchain;
}

void fat32_set_direntry(uint32_t* ptr, const char* filename, uint32_t filechain,
                        uint32_t filesize, uint32_t flags)
{
    memcpy(ptr, filename, 11);
    ((uint16_t*)ptr)[0xa] = filechain >> 16;
    ((uint8_t*)ptr)[0xb] = flags;
    ((uint16_t*)ptr)[0xa] = filechain >> 16;
    ((uint16_t*)ptr)[0xd] = filechain & 0xffff;
    ptr[7] = filesize;
}

uint32_t fat32_store_direntry(uint32_t dirchain, const char* filename,
                              uint32_t filechain, uint32_t filesize, uint32_t flags)
{
    uint32_t i, j;
    uint32_t lastidx;
    while (dirchain > 1 && dirchain < 0x0ffffff0)
    {
        uint32_t sector = (dirchain - 2) * fat32_secperclus + fat32_database;
        for (j = 0; j < fat32_secperclus; j++)
        {
            if (storage_read(sector + j, 1, fat32_buf1)) return 1;
            for (i = 0; i < 0x200; i += 8)
                if (((uint8_t*)fat32_buf1)[i << 2] == 0
                 || ((uint8_t*)fat32_buf1)[i << 2] == 0xe5)
                {
                    fat32_set_direntry(&fat32_buf1[i], filename, filechain, filesize, flags);
                    if (storage_write(sector + j, 1, fat32_buf1)) return 1;
                    return 0;
                }
        }
        uint32_t fatsector = fat32_fatbase + (dirchain >> 9);
        if (storage_read(fatsector, 1, fat32_buf1)) return 1;
        lastidx = dirchain;
        dirchain = fat32_buf1[(i << 9) + (dirchain & 0x1FF)];
    }
    uint32_t scanidx = 2;
    uint32_t scansect = 0xffffffff;
    while (scanidx < fat32_clustercount + 2)
    {
        if ((scanidx >> 9) != scansect)
        {
            scansect = scanidx >> 9;
            if (storage_read(fat32_fatbase + scansect, 1, fat32_buf1)) return 1;
        }
        if (!fat32_buf1[scanidx & 0x1ff]) break;
        scanidx++;
    }
    if (scanidx >= fat32_clustercount + 2) return 1;
    fat32_buf1[scanidx & 0x1ff] = 0x0fffffff;
    if (storage_write(fat32_fatbase + scansect, 1, fat32_buf1)) return 1;
    if (storage_read(fat32_fatbase + (lastidx >> 9), 1, fat32_buf1)) return 1;
    fat32_buf1[lastidx & 0x1ff] = scanidx;
    if (storage_write(fat32_fatbase + scansect, 1, fat32_buf1)) return 1;
    uint32_t sector = (scanidx - 2) * fat32_secperclus + fat32_database;
    for (i = 0; i < fat32_secperclus; i++)
    {
        memset(fat32_buf1, 0, 0x800);
        if (!i) fat32_set_direntry(fat32_buf1, filename, filechain, filesize, flags);
        if (storage_write(sector + i, 1, fat32_buf1)) return 1;
    }
    return 0;
}

uint32_t fat32_create_dir(uint32_t parent, const char* dirname)
{
    uint32_t i;
    uint32_t scanidx = 2;
    uint32_t scansect = 0xffffffff;
    while (scanidx < fat32_clustercount + 2)
    {
        if ((scanidx >> 9) != scansect)
        {
            scansect = scanidx >> 9;
            if (storage_read(fat32_fatbase + scansect, 1, fat32_buf1)) return 0;
        }
        if (!fat32_buf1[scanidx & 0x1ff]) break;
        scanidx++;
    }
    if (scanidx >= fat32_clustercount + 2) return 0;
    fat32_buf1[scanidx & 0x1ff] = 0x0fffffff;
    if (storage_write(fat32_fatbase + scansect, 1, fat32_buf1)) return 0;
    fat32_store_direntry(parent, dirname, scanidx, 0, 0x10);
    uint32_t sector = (scanidx - 2) * fat32_secperclus + fat32_database;
    for (i = 0; i < fat32_secperclus; i++)
    {
        memset(fat32_buf1, 0, 0x800);
        if (!i)
        {
            fat32_set_direntry(fat32_buf1, ".          ", scanidx, 0, 0x10);
            if (parent == fat32_rootdirclus) parent = 0;
            fat32_set_direntry(&fat32_buf1[8], "..         ", parent, 0, 0x10);
        }
        if (storage_write(sector + i, 1, fat32_buf1)) return 0;
    }
    return scanidx;
}

uint32_t fat32_read_file(const char* filename, uint32_t maxsize, void* buffer, uint32_t* filesize)
{
}

uint32_t fat32_get_partition_start()
{
    return fat32_startsector;
}

uint32_t fat32_init()
{
    uint32_t i;
    fat32_ok = 0;
    fat32_startsector = 0xFFFFFFFF;
    if (storage_init()) return 1;

    if (storage_read(0, 1, fat32_buf1)) return 1;

    if (*((uint16_t*)((uint32_t)fat32_buf1 + 0x1FE)) != 0xAA55)
    {
        return 1;
    }

    for (i = 0x1C2; i < 0x200; i += 0x10)
        if (((uint8_t*)fat32_buf1)[i] == 0xB)
        {
            fat32_startsector = *((uint16_t*)((uint32_t)fat32_buf1 + i + 4))
                              | (*((uint16_t*)((uint32_t)fat32_buf1 + i + 6)) << 16);
            break;
        }

    if (fat32_startsector == 0xFFFFFFFF
     && *((uint16_t*)((uint32_t)fat32_buf1 + 0x52)) == 0x4146
     && *((uint8_t*)((uint32_t)fat32_buf1 + 0x54)) == 0x54)
        fat32_startsector = 0;

    if (fat32_startsector == 0xFFFFFFFF) return 1;

    if (storage_read(fat32_startsector, 1, fat32_buf1)) return 1;

    if (*((uint16_t*)((uint32_t)fat32_buf1 + 0x1FE)) != 0xAA55) return 1;

    if (((uint8_t*)fat32_buf1)[0xB] != 0 || ((uint8_t*)fat32_buf1)[0xC] != 8) return 1;

    fat32_secperclus = ((uint8_t*)fat32_buf1)[0xD];
    uint32_t reserved = ((uint16_t*)fat32_buf1)[0x7];
    fat32_fatcount = ((uint8_t*)fat32_buf1)[0x10];

    if (((uint8_t*)fat32_buf1)[0x11] != 0) return 1;

    fat32_sectorcount = fat32_buf1[8];
    fat32_fatsize = fat32_buf1[9];

    if (((uint16_t*)fat32_buf1)[0x15] != 0) return 1;

    fat32_rootdirclus = fat32_buf1[0xB];

    fat32_clustercount = (fat32_sectorcount - reserved
                        - fat32_fatcount * fat32_fatsize) / fat32_secperclus;

    fat32_fatbase = fat32_startsector + reserved;
    fat32_database = fat32_fatbase + fat32_fatcount * fat32_fatsize;

    fat32_ok = 1;
    return 0;
}
