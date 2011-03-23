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


#include "emcorelib.h"
#include "export/libmkfat32.h"


struct libmkfat32_api apitable =
{
    .mkfat32 = mkfat32
};


EMCORE_LIB_HEADER(LIBMKFAT32_IDENTIFIER, LIBMKFAT32_API_VERSION, LIBMKFAT32_MIN_API_VERSION,
                  NULL, NULL, apitable)


int mkfat32(int volume, int startsector, int totalsectors, int sectorsize, int secperclus,
            const char* label, void* statususer, void (*statusinit)(void* user, int max),
            void (*statuscallback)(void* user, int current))
{
    uint32_t i, j;
    uint32_t rootdirclus = 2;
    uint32_t fatsectors = 1;
    uint32_t oldfatsectors = 0;
    uint32_t clustercount;
    uint32_t reserved = 2;
    disk_unmount(volume);
    while (fatsectors != oldfatsectors)
    {
        oldfatsectors = fatsectors;
        clustercount = (totalsectors - fatsectors - reserved) / secperclus;
        fatsectors = (clustercount * 4 + sectorsize + 8)  / sectorsize;
    }
    uint32_t database = fatsectors + reserved;
    uint32_t clusoffset = 0;
    uint32_t* buf = memalign(0x10, 32 * sectorsize);
    memset(buf, 0, sectorsize);
    memcpy(buf, "\xeb\x58\x00MSWIN5.0", 0xb);
    ((uint8_t*)buf)[0xb] = sectorsize & 0xff;
    ((uint8_t*)buf)[0xc] = sectorsize >> 8;
    ((uint8_t*)buf)[0xd] = secperclus;
    ((uint16_t*)buf)[7] = reserved;
    memcpy(&((uint8_t*)buf)[0x10], "\x01\0\0\0\0\xf8\0\0\x3f\0\xff", 0xb);
    buf[8] = startsector;
    buf[8] = totalsectors;
    buf[9] = fatsectors;
    buf[0xb] = rootdirclus + clusoffset;
    ((uint16_t*)buf)[0x18] = 1;
    ((uint8_t*)buf)[0x40] = 0x80;
    ((uint8_t*)buf)[0x42] = 0x29;
    memcpy(&((uint8_t*)buf)[0x47], label, 0xb);
    memcpy(&((uint8_t*)buf)[0x52], "FAT32   ", 8);
    ((uint16_t*)buf)[0xff] = 0xaa55;
    PASS_RC_FREE(storage_write_sectors_md(volume, startsector, 1, buf), 2, 0, buf);
    memset(buf, 0, sectorsize);
    buf[0] = 0x41615252;
    buf[0x79] = 0x61417272;
    buf[0x7a] = clustercount - 1;
    buf[0x7b] = 2;
    buf[0x7f] = 0xaa550000;
    PASS_RC_FREE(storage_write_sectors_md(volume, startsector + 1, 1, buf), 2, 1, buf);
    statusinit(statususer, fatsectors);
    uint32_t cursect = 0;
    for (i = 0; i < fatsectors; i += 32)
    {
        memset(buf, 0, 32 * sectorsize);
        if (!i) memcpy(buf, "\xf8\xff\xff\x0f\xff\xff\xff\xff\xff\xff\xff\x0f", 12);
        PASS_RC_FREE(storage_write_sectors_md(volume, startsector + reserved + i,
                                         MIN(fatsectors - i, 32), buf), 2, 2, buf);
        statuscallback(statususer, i);
    }
    memset(buf, 0, secperclus * sectorsize);
    memcpy(buf, label, 11);
    ((uint8_t*)buf)[0xc] = 0x80;
    PASS_RC_FREE(storage_write_sectors_md(volume, startsector + database,
                                          secperclus, buf), 2, 3, buf);
    free(buf);
    disk_mount(volume);
}
