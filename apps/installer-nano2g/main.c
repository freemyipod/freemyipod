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


#include "embiosapp.h"


void main();
EMBIOS_APP_HEADER("Installer thread", 0x10000, main, 127)


uint16_t lcdbuffer[176 * 132];
uint16_t backdrop[176 * 132];
#define BMPIDX_BACKDROP 0
#define BMPIDX_WELCOME 1
#define BMPIDX_BADPARTITION 2
#define BMPIDX_CANCELLED 3
#define BMPIDX_REPARTITION 4
#define BMPIDX_INSTALLING 5
#define BMPIDX_PREPARING 6
#define BMPIDX_REPARTITIONING 7
#define BMPIDX_INSTALLFILES 8
#define BMPIDX_FLASHING 9

struct wakeup eventwakeup;
volatile int button;

char mallocbuf[0xec0000] __attribute__((aligned(16)));
tlsf_pool mallocpool;

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

#define nor ((uint8_t*)0x24000000)
#define norword ((uint32_t*)0x24000000)

extern uint32_t _scriptstart;


void handler(enum button_event eventtype, int which, int value)
{
    if (eventtype == BUTTON_PRESS) button |= 1 << which;
    wakeup_signal(&eventwakeup);
}

void* malloc(size_t size)
{
    void* result = tlsf_malloc(mallocpool, size);
    if (!result && size) panic(PANIC_KILLTHREAD, "Out of memory!");
    return result;
}

void* memalign(size_t align, size_t size)
{
    void* result = tlsf_memalign(mallocpool, align, size);
    if (!result && size) panic(PANIC_KILLTHREAD, "Out of memory!");
    return result;
}

void* realloc(void* ptr, size_t size)
{
    void* result = tlsf_realloc(mallocpool, ptr, size);
    if (!result && size) panic(PANIC_KILLTHREAD, "Out of memory!");
    return result;
}

void free(void* ptr)
{
    tlsf_free(mallocpool, ptr);
}

uint32_t freeret(uint32_t rc, void* ptr)
{
    tlsf_free(mallocpool, ptr);
    return rc;
}

int decryptfw(void* image, uint32_t offset)
{
    uint32_t size = ((uint32_t*)image)[5];
    if (size > 0x800000) return 0;
    hwkeyaes(HWKEYAES_DECRYPT, ((uint32_t*)image)[2], &((uint8_t*)image)[offset], size);
    memcpy(image, &((uint8_t*)image)[offset], size);
    return size;
}

uint32_t getfw(const char* filename, uint32_t* sector, int* size)
{
    uint32_t i;
    uint32_t* buffer = tlsf_memalign(mallocpool, 0x10, 0x800);
    if (storage_read_sectors_md(0, 0, 1, buffer) != 0) return freeret(1, buffer);
    if (*((uint16_t*)((uint32_t)buffer + 0x1FE)) != 0xAA55) return freeret(1, buffer);
    uint32_t startsector = 0;
    for (i = 0x1C2; i < 0x200; i += 0x10)
        if (((uint8_t*)buffer)[i] == 0)
        {
            startsector = *((uint16_t*)((uint32_t)buffer + i + 4))
                        | (*((uint16_t*)((uint32_t)buffer + i + 6)) << 16);
            break;
        }
    if (startsector == 0) return freeret(1, buffer);
    if (storage_read_sectors_md(0, startsector, 1, buffer) != 0) return freeret(1, buffer);
    if (buffer[0x40] != 0x5B68695D) return freeret(1, buffer);
    if (storage_read_sectors_md(0, startsector + 1 + (buffer[0x41] >> 11), 1, buffer) != 0)
        return freeret(1, buffer);
    for (i = 0; i < 0x1FE; i += 10)
        if (memcmp(&buffer[i], filename, 8) == 0)
        {
            *sector = startsector + (buffer[i + 3] >> 11);
            *size = buffer[i + 4] + 0x800;
            tlsf_free(mallocpool, buffer);
            return 0;
        }
    return freeret(2, buffer);
}

uint32_t readfw(const char* filename, void** address, int* size)
{
    uint32_t sector;
    uint32_t rc = getfw(filename, &sector, size);
    if (rc) return rc;
    *address = tlsf_memalign(mallocpool, 0x10, *size);
    if (storage_read_sectors_md(0, sector, ((*size + 0x7FF) >> 11), *address) != 0)
        return freeret(1, *address);
    *size = decryptfw(*address, 0x800);
    tlsf_realloc(mallocpool, *address, *size);
    return 0;
}

uint32_t getapplenor(const char* filename, void** address, int* size)
{
    uint32_t i;
    for (i = 0xffe00; i < 0x100000; i += 0x28)
        if (memcmp(&nor[i], filename, 8) == 0)
        {
            *size = norword[(i + 0x10) >> 2] + 0x200;
            *address = &nor[norword[(i + 0xc) >> 2]];
            return 0;
        }
    return 1;
}

uint32_t readapplenor(const char* filename, void** address, int* size)
{
    void* noraddr;
    uint32_t rc = getapplenor(filename, &noraddr, size);
    if (rc) return rc;
    *address = malloc(*size);
    memcpy(*address, noraddr, *size);
    *size = decryptfw(*address, 0x200);
    realloc(*address, *size);
    return 0;
}

uint32_t fat32_resize_patchdirs(uint32_t clusterchain, uint32_t clustoffset,
                                struct progressbar_state* progressbar, int min, int len)
{
    uint32_t i, j,  rc;
    uint32_t* buffer = (uint32_t*)memalign(0x10, 0x800);
    int pos = min, newlen = len / 15;
    while (clusterchain < 0x0ffffff0)
    {
        uint32_t sectorbase = (clusterchain - 2) * fat32_secperclus + fat32_database;
        for (i = 0; i < fat32_secperclus; i++)
        {
            if (storage_read_sectors_md(0, sectorbase + i, 1, buffer))
            {
                free(buffer);
                return 2;
            }
            for (j = 0; j < 64; j++)
                if (!((uint8_t*)buffer)[i << 2])
                {
                    free(buffer);
                    return 0;
                }
                else if (((uint8_t*)buffer)[j << 5] == 0xe5) continue;
                else if (((uint8_t*)buffer)[(j << 5) + 11] & 8) continue;
                else
                {
                    uint32_t clust = (((uint16_t*)buffer)[(j << 4) + 0xa] << 16)
                                    | ((uint16_t*)buffer)[(j << 4) + 0xd];
                    if (clust > 1 && clust < 0xffffff0)
                    {
                        clust += clustoffset;
                        ((uint16_t*)buffer)[(j << 4) + 0xa] = clust >> 16;
                        ((uint16_t*)buffer)[(j << 4) + 0xd] = clust & 0xffff;
                        if ((((uint8_t*)buffer)[(j << 5) + 0xb] & 0x10)
                         && memcmp(&((uint8_t*)buffer)[j << 5], ".          ", 11)
                         && memcmp(&((uint8_t*)buffer)[j << 5], "..         ", 11))
                            if ((rc = fat32_resize_patchdirs(clust, clustoffset,
                                                             progressbar, pos, newlen)))
                            {
                                free(buffer);
                                return rc;
                            }
                        pos += newlen;
                        newlen = 15 * newlen / 16;
                    }
                }
            if (storage_write_sectors_md(0, sectorbase + i, 1, buffer))
            {
                free(buffer);
                return 2;
            }
        }
        uint32_t fatsector = fat32_fatbase + (clusterchain >> 9);
        if (storage_read_sectors_md(0, fatsector, 1, buffer))
        {
            free(buffer);
            return 2;
        }
        clusterchain = buffer[(i << 9) + (clusterchain & 0x1FF)];
    }
    free(buffer);
    if (len) progressbar_setpos(progressbar, min + len, false);
    return 0;
}

uint32_t fat32_resize_fulldisk(struct progressbar_state* progressbar)
{
    uint32_t i, j, rc;
    uint32_t fatsectors = 1;
    uint32_t oldfatsectors = 0;
    uint32_t clustercount;
    uint32_t reserved;
    struct storage_info storageinfo;
    storage_get_info(0, &storageinfo);
    uint32_t totalsectors = storageinfo.num_sectors;
    uint32_t* buf1 = (uint32_t*)memalign(0x10, 0x800);
    uint32_t* buf2 = (uint32_t*)memalign(0x10, 0x800);
    if (!fat32_ok)
    {
        fat32_secperclus = 4;
        fat32_rootdirclus = 2;
    }
    while (fatsectors != oldfatsectors)
    {
        oldfatsectors = fatsectors;
        if (!fat32_ok) reserved = 2;
        else reserved = (fat32_database - fatsectors - 2) % fat32_secperclus + 2;
        clustercount = (totalsectors - fatsectors - reserved) / fat32_secperclus;
        fatsectors = (clustercount + 513) >> 9;
    }
    uint32_t database = fatsectors + reserved;
    uint32_t clusoffset;
    if (!fat32_ok) clusoffset = 0;
    else clusoffset = (fat32_database - database) / fat32_secperclus;
    memset(buf1, 0, 0x800);
    if (fat32_ok)
        if (storage_read_sectors_md(0, fat32_startsector, 1, buf2))
        {
            fat32_ok = 0;
            free(buf1);
            free(buf2);
            return 2;
        }
    memcpy(buf1, "\xeb\x58\x00MSWIN5.0\0\x08", 0xd);
    ((uint8_t*)buf1)[0xd] = fat32_secperclus;
    ((uint16_t*)buf1)[7] = reserved;
    memcpy(&((uint8_t*)buf1)[0x10], "\x01\0\0\0\0\xf8\0\0\x3f\0\xff", 0xb);
    buf1[8] = totalsectors;
    buf1[9] = fatsectors;
    buf1[0xb] = fat32_rootdirclus + clusoffset;
    ((uint16_t*)buf1)[0x18] = 1;
    ((uint8_t*)buf1)[0x40] = 0x80;
    ((uint8_t*)buf1)[0x42] = 0x29;
    if (!fat32_ok) memcpy(&((uint8_t*)buf1)[0x43], "\0\0\0\0iPod Nano  ", 0xf);
    else memcpy(&((uint8_t*)buf1)[0x43], &((uint8_t*)buf2)[0x43], 0xf);
    memcpy(&((uint8_t*)buf1)[0x52], "FAT32   ", 8);
    ((uint16_t*)buf1)[0xff] = 0xaa55;
    if (storage_write_sectors_md(0, 0, 1, buf1))
    {
        fat32_ok = 0;
        free(buf1);
        free(buf2);
        return 2;
    }
    if (fat32_ok)
    {
        if (storage_read_sectors_md(0, fat32_startsector + ((uint16_t*)buf2)[0x18], 1, buf1))
        {
            fat32_ok = 0;
            free(buf1);
            free(buf2);
            return 2;
        }
        buf1[0x7a] += clustercount - fat32_clustercount;
    }
    else
    {
        memset(buf1, 0, 0x800);
        buf1[0] = 0x41615252;
        buf1[0x79] = 0x61417272;
        buf1[0x7a] = clustercount - 1;
        buf1[0x7b] = 2;
        buf1[0x7f] = 0xaa550000;
    }
    if (storage_write_sectors_md(0, 1, 1, buf1))
    {
        fat32_ok = 0;
        free(buf1);
        free(buf2);
        return 2;
    }
    progressbar_setpos(progressbar, 5, false);
    uint32_t cursect = 0;
    if (!fat32_ok)
    {
        for (i = 0; i < fatsectors; i++)
        {
            memset(buf1, 0, 0x800);
            if (!i) memcpy(buf1, "\xf8\xff\xff\x0f\xff\xff\xff\xff\xff\xff\xff\x0f", 12);
            if (storage_write_sectors_md(0, reserved + i, 1, buf1))
            {
                free(buf1);
                free(buf2);
                return 2;
            }
            progressbar_setpos(progressbar, 5 + i * 90 / fatsectors, false);
        }
    }
    else
    {
        for (i = 0; i < fatsectors; i++)
        {
            memset(buf1, 0, 0x800);
            for (j = 0; j < 512; j++)
            {
                if (!i && !j) buf1[j] = 0x0fffffff;
                else if (!i && j == 1) buf1[j] = 0xffffffff;
                else if (((i << 9) | j) < clusoffset + 2);
                else if (((i << 9) | j) >= clusoffset + fat32_clustercount + 2);
                else
                {
                    uint32_t oldclust = (((i << 9) | j) - clusoffset);
                    if (((oldclust >> 9) + fat32_fatbase) != cursect)
                    {
                        cursect = (oldclust >> 9) + fat32_fatbase;
                        if (storage_read_sectors_md(0, cursect, 1, buf2))
                        {
                            fat32_ok = 0;
                            free(buf1);
                            free(buf2);
                            return 2;
                        }
                    }
                    buf1[j] = buf2[oldclust & 0x1ff];
                    if (buf1[j] > 1 && buf1[j] < 0xffffff0)
                        buf1[j] += clusoffset;
                }
            }
            if (storage_write_sectors_md(0, reserved + i, 1, buf1))
            {
                fat32_ok = 0;
                free(buf1);
                free(buf2);
                return 2;
            }
            progressbar_setpos(progressbar, 5 + i * 20 / fatsectors, false);
        }
    }
    fat32_startsector = 0;
    fat32_database = database;
    fat32_fatbase = reserved;
    fat32_fatsize = fatsectors;
    fat32_fatcount = 1;
    fat32_sectorcount = totalsectors;
    fat32_clustercount = clustercount;
    fat32_rootdirclus = fat32_rootdirclus + clusoffset;
    if (!fat32_ok)
    {
        for (i = 0; i < fat32_secperclus; i++)
        {
            memset(buf1, 0, 0x800);
            if (!i) memcpy(buf1, "iPod Nano  \x08", 12);
            if (storage_write_sectors_md(0, database + i, 1, buf1))
            {
                free(buf1);
                free(buf2);
                return 2;
            }
        }
        free(buf1);
        free(buf2);
    }
    else
    {
        free(buf1);
        free(buf2);
        if ((rc = fat32_resize_patchdirs(fat32_rootdirclus, clusoffset, progressbar, 25, 75)))
        {
            fat32_ok = 0;
            return rc;
        }
    }
    progressbar_setpos(progressbar, 100, false);
    fat32_ok = 1;
    return 0;
}

uint32_t fat32_init()
{
    uint32_t i;
    fat32_ok = 0;
    fat32_startsector = 0xFFFFFFFF;
    uint32_t* buf = (uint32_t*)memalign(0x10, 0x800);

    if (storage_read_sectors_md(0, 0, 1, buf)) return freeret(2, buf);

    if (*((uint16_t*)((uint32_t)buf + 0x1FE)) != 0xAA55) return 1;

    for (i = 0x1C2; i < 0x200; i += 0x10)
        if (((uint8_t*)buf)[i] == 0xB)
        {
            fat32_startsector = *((uint16_t*)((uint32_t)buf + i + 4))
                              | (*((uint16_t*)((uint32_t)buf + i + 6)) << 16);
            break;
        }

    if (fat32_startsector == 0xFFFFFFFF
     && *((uint16_t*)((uint32_t)buf + 0x52)) == 0x4146
     && *((uint8_t*)((uint32_t)buf + 0x54)) == 0x54)
        fat32_startsector = 0;

    if (fat32_startsector == 0xFFFFFFFF) return freeret(1, buf);

    if (storage_read_sectors_md(0, fat32_startsector, 1, buf)) return freeret(2, buf);

    if (*((uint16_t*)((uint32_t)buf + 0x1FE)) != 0xAA55) return freeret(1, buf);

    if (((uint8_t*)buf)[0xB] != 0 || ((uint8_t*)buf)[0xC] != 8) return freeret(1, buf);

    fat32_secperclus = ((uint8_t*)buf)[0xD];
    uint32_t reserved = ((uint16_t*)buf)[0x7];
    fat32_fatcount = ((uint8_t*)buf)[0x10];

    if (((uint8_t*)buf)[0x11] != 0) return freeret(1, buf);

    fat32_sectorcount = buf[8];
    fat32_fatsize = buf[9];

    if (((uint16_t*)buf)[0x15] != 0) return freeret(1, buf);

    fat32_rootdirclus = buf[0xB];
    free(buf);

    fat32_clustercount = (fat32_sectorcount - reserved
                        - fat32_fatcount * fat32_fatsize) / fat32_secperclus;

    fat32_fatbase = fat32_startsector + reserved;
    fat32_database = fat32_fatbase + fat32_fatcount * fat32_fatsize;

    fat32_ok = 1;
    return 0;
}

void main(void)
{
    uint32_t i, j, k, rc;
    void* bitmapdata[10];
    uint32_t bitmapsize[10];
    uint32_t* script;
#define scriptb ((uint8_t*)script)
    uint32_t dummy;
    struct progressbar_state progressbar;
    bool repartition = false;
    bool appleflash;
    void* syscfgptr;
    int osossize = 0;
    void* ososptr;
    int diaguclsize = 0;
    void* diaguclptr;
    int diskuclsize = 0;
    void* diskuclptr;
    uint8_t* norbuf;
#define norbufword ((uint32_t*)norbuf)

    button = 0;
    wakeup_init(&eventwakeup);
    button_register_handler(handler);
    mallocpool = tlsf_create(mallocbuf, sizeof(mallocbuf));

    script = &_scriptstart;
    for (i = 0; i < 10; i++)
    {
        bitmapsize[i] = *script;
        bitmapdata[i] = &script[1];
        script = &script[1 + (bitmapsize[i] >> 2)];
    }

    void* bmpbuffer = malloc(0xb600);
    ucl_decompress(bitmapdata[BMPIDX_BACKDROP], bitmapsize[BMPIDX_BACKDROP], bmpbuffer, &dummy);
    renderbmp(backdrop, bmpbuffer, 176);
    memcpy(lcdbuffer, backdrop, 0xb580);
    ucl_decompress(bitmapdata[BMPIDX_WELCOME], bitmapsize[BMPIDX_WELCOME], bmpbuffer, &dummy);
    renderbmp(&lcdbuffer[176 * 25 + 25], bmpbuffer, 176);
    displaylcd(0, 175, 0, 131, lcdbuffer, 0);
    backlight_set_fade(32);
    backlight_set_brightness(177);
    backlight_on(true);
    if (norword[0x400] == 0x53436667) appleflash = false;
    else if (norword[0x1000] == 0x53436667) appleflash = true;
    else panic(PANIC_KILLTHREAD, "Boot flash contents are damaged! "
                                 "(No SYSCFG found)\n\nPlease ask for help.\n");
    disk_unmount(0);
    rc = fat32_init();
    if (rc == 2) panic(PANIC_KILLTHREAD, "Data flash I/O error!");
    sleep(5000000);
    if (rc)
    {
        ucl_decompress(bitmapdata[BMPIDX_BADPARTITION], bitmapsize[BMPIDX_BADPARTITION],
                       bmpbuffer, &dummy);
        memcpy(lcdbuffer, backdrop, 0xb580);
        renderbmp(lcdbuffer, bmpbuffer, 176);
        displaylcd(0, 175, 0, 131, lcdbuffer, 0);
        while (true)
        {
            wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
            if (button == 2)
            {
                repartition = true;
                break;
            }
            else if (button == 4)
            {
                ucl_decompress(bitmapdata[BMPIDX_CANCELLED], bitmapsize[BMPIDX_CANCELLED],
                               bmpbuffer, &dummy);
                memcpy(lcdbuffer, backdrop, 0xb580);
                renderbmp(lcdbuffer, bmpbuffer, 176);
                displaylcd(0, 175, 0, 131, lcdbuffer, 0);
                sleep(500000);
                button = 0;
                while (!button) wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
                memcpy((void*)0x2202bf00, "diskmodehotstuff\1\0\0", 20);
                shutdown(false);
                reset();
            }
            button = 0;
        }
    }
    else if (fat32_startsector != 0)
    {
        ucl_decompress(bitmapdata[BMPIDX_REPARTITION], bitmapsize[BMPIDX_REPARTITION],
                       bmpbuffer, &dummy);
        memcpy(lcdbuffer, backdrop, 0xb580);
        renderbmp(lcdbuffer, bmpbuffer, 176);
        displaylcd(0, 175, 0, 131, lcdbuffer, 0);
        while (true)
        {
            wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
            if (button == 2)
            {
                repartition = true;
                break;
            }
            else if (button == 4) break;
            button = 0;
        }
    }
    ucl_decompress(bitmapdata[BMPIDX_INSTALLING], bitmapsize[BMPIDX_INSTALLING],
                   bmpbuffer, &dummy);
    renderbmp(backdrop, bmpbuffer, 176);
    ucl_decompress(bitmapdata[BMPIDX_PREPARING], bitmapsize[BMPIDX_PREPARING],
                    bmpbuffer, &dummy);
    memcpy(lcdbuffer, backdrop, 0xb580);
    renderbmp(&lcdbuffer[176 * 36], bmpbuffer, 176);
    displaylcd(0, 175, 0, 131, lcdbuffer, 0);
    free(bmpbuffer);
    progressbar_init(&progressbar, 15, 160, 50, 60, 0xce79, 0x18e3, 0x7bf9, 0, 100);

    syscfgptr = malloc(0x1000);
    if (appleflash)
    {
        memcpy(syscfgptr, &nor[0x4000], 0x1000);
        if (readapplenor("hslfksid", &diskuclptr, &diskuclsize)) diskuclsize = 0;
        else
        {
            progressbar_setpos(&progressbar, 5, false);
            void* newptr = malloc(diskuclsize + (diskuclsize >> 3) + 256);
            if (ucl_nrv2e_99_compress(diskuclptr, diskuclsize, newptr,
                                      (uint32_t*)&diskuclsize, 0, 10, 0, 0))
            {
                free(newptr);
                diskuclsize = 0;
            }
            free(diskuclptr);
            realloc(newptr, diskuclsize);
            diskuclptr = newptr;
        }
        progressbar_setpos(&progressbar, 35, false);
        if (readapplenor("hslfgaid", &diaguclptr, &diaguclsize)) diaguclsize = 0;
        else
        {
            progressbar_setpos(&progressbar, 40, false);
            void* newptr = malloc(diaguclsize + (diaguclsize >> 3) + 256);
            if (ucl_nrv2e_99_compress(diaguclptr, diaguclsize, newptr,
                                      (uint32_t*)&diaguclsize, 0, 10, 0, 0))
            {
                free(newptr);
                diaguclsize = 0;
            }
            free(diaguclptr);
            realloc(newptr, diaguclsize);
            diaguclptr = newptr;
        }
        progressbar_setpos(&progressbar, 70, false);
        if (readfw("DNANkbso", &ososptr, &osossize)) osossize = 0;
        if (osossize)
        {
            if (((uint8_t*)ososptr)[0x64d48] == 0x2b && ((uint8_t*)ososptr)[0x64d54] == 0x34)
            {
                ((uint8_t*)ososptr)[0x64d48] = 0x43;
                ((uint8_t*)ososptr)[0x64d54] = 0x52;
            }
            if (((uint8_t*)ososptr)[0x3acd8] == 0x01)
                ((uint8_t*)ososptr)[0x3acd8] = 0x00;
        }
        progressbar_setpos(&progressbar, 90, false);
    }
    else
    {
        memcpy(syscfgptr, &nor[0x1000], 0x1000);
        diskuclsize = bootflash_filesize("diskmode");
        if (diskuclsize > 0)
        {
            diskuclptr = bootflash_getaddr("diskmode");
            if (!(bootflash_attributes("diskmode") & 0x800))
            {
                void* newptr = malloc(diskuclsize + (diskuclsize >> 3) + 256);
                if (ucl_nrv2e_99_compress(diskuclptr, diskuclsize, newptr,
                                          (uint32_t*)&diskuclsize, 0, 10, 0, 0))
                {
                    free(newptr);
                    diskuclsize = 0;
                }
                realloc(newptr, diskuclsize);
                diskuclptr = newptr;
            }
        }
        progressbar_setpos(&progressbar, 45, false);
        diaguclsize = bootflash_filesize("diagmode");
        if (diaguclsize > 0)
        {
            diaguclptr = bootflash_getaddr("diagmode");
            if (!(bootflash_attributes("diagmode") & 0x800))
            {
                void* newptr = malloc(diaguclsize + (diaguclsize >> 3) + 256);
                if (ucl_nrv2e_99_compress(diaguclptr, diaguclsize, newptr,
                                          (uint32_t*)&diaguclsize, 0, 10, 0, 0))
                {
                    free(newptr);
                    diaguclsize = 0;
                }
                realloc(newptr, diaguclsize);
                diaguclptr = newptr;
            }
        }
        progressbar_setpos(&progressbar, 90, false);
    }
    norbuf = malloc(0x100000);
    memset(norbuf, 0xff, 0x100000);
    memcpy(&norbuf[0x1000], syscfgptr, 0x1000);
    free(syscfgptr);
    uint32_t sp = 0;
    uint32_t beginptr = 0x2000;
    uint32_t endptr = 0x100000;
    uint32_t dirptr = 0;
    while (script[sp])
    {
        uint32_t file = script[sp] & 0xff;
        uint32_t flags = (script[sp] >> 8) & 0xff;
        uint32_t align = (script[sp] >> 16) & 0xff;
        void* data;
        uint32_t size;
        sp++;
        switch (file)
        {
            case 1:
                data = diskuclptr;
                size = diskuclsize;
                flags |= 2;
                break;
            case 2:
                data = diaguclptr;
                size = diaguclsize;
                flags |= 2;
                break;
            default:
                data = &scriptb[script[sp++]];
                size = script[sp++];
        }
        if (size)
        {
            if (align && !(flags & 1))
            {
                if ((align << 12) < beginptr)
                    panicf(PANIC_KILLTHREAD, "Error: Align failed! (%02X)", align);
                beginptr = align << 12;
            }
            if (endptr - beginptr < size)
                panicf(PANIC_KILLTHREAD, "Error: Flash is full!");
            uint32_t storesize = size;
            if (flags & 2) storesize |= 0x80000000;
            if (flags & 1)
            {
                endptr -= ((size + 0xfff) & ~0xfff);
                memcpy(&norbuf[endptr], data, size);
                file = endptr;
            }
            else
            {
                memcpy(&norbuf[beginptr], data, size);
                file = beginptr;
                beginptr += ((size + 0xfff) & ~0xfff);
            }
            if (!(flags & 4))
            {
                if (dirptr >= 0x1000)
                    panicf(PANIC_KILLTHREAD, "Error: Directory is full!");
                memcpy(&norbuf[dirptr], &script[sp], 8);
                norbufword[(dirptr >> 2) + 2] = file;
                norbufword[(dirptr >> 2) + 3] = storesize;
                dirptr += 0x10;
                sp += 2;
            }
        }
        else if (!(flags & 4)) sp += 2;
    }
    progressbar_setpos(&progressbar, 100, false);
    if (diskuclptr && (uint32_t)diskuclptr < 0x24000000) free(diskuclptr);
    if (diaguclptr && (uint32_t)diaguclptr < 0x24000000) free(diaguclptr);

    if (repartition)
    {
        bmpbuffer = malloc(0xb600);
        memcpy(lcdbuffer, backdrop, 0xb580);
        ucl_decompress(bitmapdata[BMPIDX_REPARTITIONING], bitmapsize[BMPIDX_REPARTITIONING],
                       bmpbuffer, &dummy);
        renderbmp(&lcdbuffer[176 * 36], bmpbuffer, 176);
        displaylcd(0, 175, 0, 131, lcdbuffer, 0);
        free(bmpbuffer);
        progressbar_init(&progressbar, 15, 160, 50, 60, 0xce79, 0x18e3, 0x7bf9, 0, 100);
        if (fat32_resize_fulldisk(&progressbar))
            panic(PANIC_KILLTHREAD, "Data flash I/O error!");
    }

    bmpbuffer = malloc(0xb600);
    memcpy(lcdbuffer, backdrop, 0xb580);
    ucl_decompress(bitmapdata[BMPIDX_INSTALLFILES], bitmapsize[BMPIDX_INSTALLFILES],
                   bmpbuffer, &dummy);
    renderbmp(&lcdbuffer[176 * 36], bmpbuffer, 176);
    displaylcd(0, 175, 0, 131, lcdbuffer, 0);
    progressbar_init(&progressbar, 15, 160, 50, 60, 0xce79, 0x18e3, 0x7bf9, 0, 100);
    disk_mount(0);
    int updating = mkdir("/iLoader");
    int status;
    if (updating)
    {
        status = script[sp + 3];
        sp = script[sp + 1] >> 2;
    }
    else
    {
        status = script[sp + 2] + script[sp + 3];
        sp += 4;
    }
    progressbar_init(&progressbar, 15, 160, 50, 60, 0xce79, 0x18e3, 0x7bf9, 0, status);
    status = 0;
    while (script[sp])
    {
        int fd;
        void* data;
        switch (script[sp])
        {
            case 1:
                mkdir(&scriptb[script[sp + 1]]);
                sp += 2;
                break;
            case 2:
                if (script[sp + 2] == 0xffffffff)
                {
                    data = ososptr;
                    script[sp + 3] = osossize;
                }
                else if (script[sp + 2] == 0xfffffffe)
                {
                    data = nor;
                    script[sp + 3] = 0x100000;
                }
                if (!script[sp + 3])
                {
                    sp += 4;
                    break;
                }
            case 3:
                fd = file_open(&scriptb[script[sp + 1]], O_RDONLY);
                if (fd >= 0)
                {
                    close(fd);
                    sp += 4;
                    break;
                }
            case 4:
                if (script[sp + 2] < 0xfffffffe) data = &scriptb[script[sp + 2]];
                fd = file_creat(&scriptb[script[sp + 1]]);
                if (fd >= 0)
                {
                    write(fd, data, script[sp + 3]);
                    close(fd);
                }
                sp += 4;
                break;
            default:
                panic(PANIC_KILLTHREAD, "Bad installation script!");
        }
        status += script[sp++];
        progressbar_setpos(&progressbar, status, false);
    }

    bmpbuffer = malloc(0xb600);
    memcpy(lcdbuffer, backdrop, 0xb580);
    ucl_decompress(bitmapdata[BMPIDX_FLASHING], bitmapsize[BMPIDX_FLASHING], bmpbuffer, &dummy);
    renderbmp(&lcdbuffer[176 * 36], bmpbuffer, 176);
    displaylcd(0, 175, 0, 131, lcdbuffer, 0);
    progressbar_init(&progressbar, 15, 160, 50, 60, 0xce79, 0x18e3, 0x7bf9, 0, 256);
    for (i = 0; i < 256; i++)
    {
        bootflash_writeraw(&norbuf[i << 12], i << 12, 1 << 12);
        progressbar_setpos(&progressbar, i, false);
    }

    shutdown(false);
    reset();
}
