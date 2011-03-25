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


#include "emcoreapp.h"
#include "libpng.h"
#include "libui.h"


#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)
#define BOOTNOTE_FILENAME "/Notes/" STR(BASENAME) ".bootnote"


void main();
EMCORE_APP_HEADER("emCORE installer", main, 127)


extern char background_png[];
extern uint32_t background_png_size;
extern char darkener_png[];
extern uint32_t darkener_png_size;
extern char disclaimer_png[];
extern uint32_t disclaimer_png_size;
extern char actions_png[];
extern uint32_t actions_png_size;
extern char f_png_emcorelib[];
extern char f_ui_emcorelib[];
extern uint32_t flashscript[];
extern uint32_t firstinstcost;
extern uint32_t firstinstscript[];
extern uint32_t commoncost;
extern uint32_t commonscript[];


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

struct wakeup eventwakeup;
volatile int button;
volatile int scrollpos;


#define nor ((uint8_t*)0x24000000)
#define norword ((uint32_t*)0x24000000)


void handler(void* user, enum button_event eventtype, int which, int value)
{
    if (eventtype == BUTTON_PRESS) button |= 1 << which;
    if (eventtype == BUTTON_RELEASE) button &= ~(1 << which);
    if (eventtype == WHEEL_MOVED_ACCEL)
        scrollpos = MAX(0, MIN(309, scrollpos + value / 8));
    wakeup_signal(&eventwakeup);
}

uint32_t freeret(uint32_t rc, void* ptr)
{
    free(ptr);
    return rc;
}

int decryptfw(void* image, uint32_t offset)
{
    uint32_t size = ((uint32_t*)image)[5];
    if (size > 0x800000) return 0;
    hwkeyaes(HWKEYAES_DECRYPT, ((uint32_t*)image)[2], &((uint8_t*)image)[offset], size);
    memcpy(image, &((uint8_t*)image)[offset], size);
    cputc(3, '.');
    return size;
}

uint32_t getfw(const char* filename, uint32_t* sector, int* size)
{
    uint32_t i;
    uint32_t* buffer = memalign(0x10, 0x800);
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
    cputc(3, '.');
    if (startsector == 0) return freeret(1, buffer);
    if (storage_read_sectors_md(0, startsector, 1, buffer) != 0) return freeret(1, buffer);
    cputc(3, '.');
    if (buffer[0x40] != 0x5B68695D) return freeret(1, buffer);
    if (storage_read_sectors_md(0, startsector + 1 + (buffer[0x41] >> 11), 1, buffer) != 0)
        return freeret(1, buffer);
    cputc(3, '.');
    for (i = 0; i < 0x1FE; i += 10)
        if (memcmp(&buffer[i], filename, 8) == 0)
        {
            *sector = startsector + (buffer[i + 3] >> 11);
            *size = buffer[i + 4] + 0x800;
            free(buffer);
            cputc(3, '.');
            return 0;
        }
    return freeret(2, buffer);
}

uint32_t readfw(const char* filename, void** address, int* size)
{
    uint32_t sector;
    uint32_t rc = getfw(filename, &sector, size);
    if (rc) return rc;
    *address = memalign(0x10, *size);
    if (storage_read_sectors_md(0, sector, ((*size + 0x7FF) >> 11), *address) != 0)
        return freeret(1, *address);
    cputc(3, '.');
    *size = decryptfw(*address, 0x800);
    realloc(*address, *size);
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
    cputc(3, '.');
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
    if (!fat32_ok) memcpy(&((uint8_t*)buf1)[0x43], "\0\0\0\0iPod Nano2G", 0xf);
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
            if (!i) memcpy(buf1, "iPod Nano2G\x08", 12);
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
    uint32_t dummy;
    int deleterc = 1;
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

    cputc(3, '.');
    struct emcorelib_header* libpng = get_library(LIBPNG_IDENTIFIER, LIBPNG_API_VERSION, LIBSOURCE_RAM_NEEDCOPY, f_png_emcorelib);
    if (!libpng) panicf(PANIC_KILLTHREAD, "Could not load PNG decoder library!");
    struct libpng_api* png = (struct libpng_api*)libpng->api;
    cputc(3, '.');
    struct emcorelib_header* libui = get_library(LIBUI_IDENTIFIER, LIBUI_API_VERSION, LIBSOURCE_RAM_NEEDCOPY, f_ui_emcorelib);
    if (!libui) panicf(PANIC_KILLTHREAD, "Could not load user interface library!");
    struct libui_api* ui = (struct libui_api*)libui->api;
    cputc(3, '.');

    struct png_info* handle = png->png_open(background_png, background_png_size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse background image!");
    cputc(3, '.');
    struct png_rgb* bg = png->png_decode_rgb(handle);
    if (!bg) panicf(PANIC_KILLTHREAD, "Could not decode background image!");
    png->png_destroy(handle);
    cputc(3, '.');
    void* darkened = malloc(176 * 132 * 3);
    if (!darkened) panicf(PANIC_KILLTHREAD, "Could not allocate darkened image!");
    cputc(3, '.');
    handle = png->png_open(darkener_png, darkener_png_size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse darkener image!");
    cputc(3, '.');
    struct png_rgba* darkener = png->png_decode_rgba(handle);
    if (!darkener) panicf(PANIC_KILLTHREAD, "Could not decode darkener image!");
    png->png_destroy(handle);
    cputc(3, '.');
    ui->blenda(176, 132, 255, darkened, 0, 0, 176, bg, 0, 0, 176, darkener, 0, 0, 176);
    free(darkener);
    cputc(3, '.');
    handle = png->png_open(disclaimer_png, disclaimer_png_size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse disclaimer image!");
    cputc(3, '.');
    struct png_rgba* disclaimer = png->png_decode_rgba(handle);
    if (!disclaimer) panicf(PANIC_KILLTHREAD, "Could not decode disclaimer image!");
    png->png_destroy(handle);
    cputc(3, '.');
    handle = png->png_open(actions_png, actions_png_size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse actions image!");
    cputc(3, '.');
    struct png_rgba* actions = png->png_decode_rgba(handle);
    if (!actions) panicf(PANIC_KILLTHREAD, "Could not decode actions image!");
    png->png_destroy(handle);
    cputc(3, '.');
    void* framebuf = malloc(160 * 91 * 3);
    if (!framebuf) panicf(PANIC_KILLTHREAD, "Could not allocate frame buffer!");
    cputc(3, '.');

    deleterc = remove(BOOTNOTE_FILENAME);
    cputc(3, '.');
    disk_unmount(0);
    rc = fat32_init();
    if (rc == 2) panic(PANIC_KILLTHREAD, "Data flash I/O error!");
    cputc(3, '.');

    if (norword[0x400] == 0x53436667) appleflash = false;
    else if (norword[0x1000] == 0x53436667) appleflash = true;
    else panic(PANIC_KILLTHREAD, "Boot flash contents are damaged! "
                                 "(No SYSCFG found)\n\nPlease ask for help.\n");


    if (appleflash)
    {
        syscfgptr = &nor[0x4000];
        if (readapplenor("hslfksid", &diskuclptr, &diskuclsize)) diskuclsize = 0;
        else
        {
            cputc(3, '.');
            void* newptr = malloc(diskuclsize + (diskuclsize >> 3) + 256);
            if (ucl_nrv2e_99_compress(diskuclptr, diskuclsize, newptr,
                                      (uint32_t*)&diskuclsize, 0, 10, 0, 0))
            {
                free(newptr);
                diskuclsize = 0;
            }
            cputc(3, '.');
            free(diskuclptr);
            realloc(newptr, diskuclsize);
            diskuclptr = newptr;
        }
        cputc(3, '.');
        if (readapplenor("hslfgaid", &diaguclptr, &diaguclsize)) diaguclsize = 0;
        else
        {
            cputc(3, '.');
            void* newptr = malloc(diaguclsize + (diaguclsize >> 3) + 256);
            if (ucl_nrv2e_99_compress(diaguclptr, diaguclsize, newptr,
                                      (uint32_t*)&diaguclsize, 0, 10, 0, 0))
            {
                free(newptr);
                diaguclsize = 0;
            }
            cputc(3, '.');
            free(diaguclptr);
            realloc(newptr, diaguclsize);
            diaguclptr = newptr;
        }
        cputc(3, '.');
        if (readfw(deleterc ? "DNANkbso" : "DNANsoso", &ososptr, &osossize)) osossize = 0;
        if (osossize)
        {
            cputc(3, '.');
            if (((uint8_t*)ososptr)[0x64d48] == 0x2b && ((uint8_t*)ososptr)[0x64d54] == 0x34)
            {
                ((uint8_t*)ososptr)[0x64d48] = 0x43;
                ((uint8_t*)ososptr)[0x64d54] = 0x52;
            }
            if (((uint8_t*)ososptr)[0x3acd8] == 0x01)
                ((uint8_t*)ososptr)[0x3acd8] = 0x00;
        }
    }
    else
    {
        syscfgptr = &nor[0x1000];
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
                cputc(3, '.');
                realloc(newptr, diskuclsize);
                diskuclptr = newptr;
            }
        }
        cputc(3, '.');
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
                cputc(3, '.');
                realloc(newptr, diaguclsize);
                diaguclptr = newptr;
            }
        }
    }
    cputc(3, '.');
    norbuf = malloc(0x100000);
    memset(norbuf, 0xff, 0x100000);
    memcpy(&norbuf[0x1000], syscfgptr, 0x1000);
    free(syscfgptr);
    cputc(3, '.');

    uint32_t* script = flashscript;
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
                data = (void*)(script[sp++]);
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
        cputc(3, '.');
    }
    if (diskuclptr && (uint32_t)diskuclptr < 0x24000000) free(diskuclptr);
    if (diaguclptr && (uint32_t)diaguclptr < 0x24000000) free(diaguclptr);
	
    if (appleflash || rc || fat32_startsector)
    {
        button = 0;
        wakeup_init(&eventwakeup);
        struct button_hook_entry* hook = button_register_handler(handler, NULL);
        if (!hook) panicf(PANIC_KILLTHREAD, "Could not register button hook!");

        displaylcd(0, 0, 176, 132, darkened, 0, 0, 176);
        backlight_set_fade(32);
        backlight_set_brightness(177);
        backlight_on(true);
        scrollpos = 0;

        while (true)
        {
            ui->blenda(160, 91, 255, framebuf, 0, 0, 160,
                       darkened, 8, 27, 176, disclaimer, 0, scrollpos, 160);
            displaylcd(8, 27, 160, 91, framebuf, 0, 0, 160);

            wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
            if (button == 0x18) break;
            else if (button == 4)
            {
                if (deleterc)
                {
                    sleep(500000);
                    ui->blenda(160, 60, 255, framebuf, 0, 0, 160,
                               darkened, 8, 27, 176, disclaimer, 0, 550, 160);
                    displaylcd(8, 27, 160, 60, framebuf, 0, 0, 160);
                    displaylcd(8, 87, 160, 31, darkened, 8, 87, 176);
                    button = 0;
                    while (!button) wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
                    memcpy((void*)0x2202bf00, "diskmodehotstuff\1\0\0", 20);
                }
                shutdown(false);
                reset();
            }
        }

        button_unregister_handler(hook);
    }

    if (rc)
    {
        ui->blenda(160, 80, 255, framebuf, 0, 0, 160,
                   darkened, 8, 27, 176, disclaimer, 0, 470, 160);
        displaylcd(8, 27, 160, 80, framebuf, 0, 0, 160);
        displaylcd(8, 107, 160, 11, darkened, 8, 107, 176);

        button = 0;
        struct button_hook_entry* hook = button_register_handler(handler, NULL);
        if (!hook) panicf(PANIC_KILLTHREAD, "Could not register button hook!");

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
                if (deleterc)
                {
                    sleep(500000);
                    ui->blenda(160, 60, 255, framebuf, 0, 0, 160,
                               darkened, 8, 27, 176, disclaimer, 0, 550, 160);
                    displaylcd(8, 27, 160, 60, framebuf, 0, 0, 160);
                    displaylcd(8, 87, 160, 31, darkened, 8, 87, 176);
                    button = 0;
                    while (!button) wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
                    memcpy((void*)0x2202bf00, "diskmodehotstuff\1\0\0", 20);
                }
                shutdown(false);
                reset();
            }
        }
		
        button_unregister_handler(hook);
    }
    else if (fat32_startsector)
    {
        ui->blenda(130, 70, 255, framebuf, 0, 0, 130,
                   darkened, 23, 27, 176, disclaimer, 0, 400, 160);
        displaylcd(8, 27, 160, 91, darkened, 8, 27, 176);
        displaylcd(23, 27, 130, 70, framebuf, 0, 0, 130);

        button = 0;
        struct button_hook_entry* hook = button_register_handler(handler, NULL);
        if (!hook) panicf(PANIC_KILLTHREAD, "Could not register button hook!");

        while (true)
        {
            wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
            if (button == 2)
            {
                repartition = true;
                break;
            }
            else if (button == 4) break;
        }
		
        button_unregister_handler(hook);
    }
	
    free(darkened);
    free(disclaimer);
            
    if (repartition)
    {
        ui->blenda(110, 20, 255, framebuf, 0, 0, 110, bg, 33, 55, 176, actions, 0, 0, 110);
        displaylcd(0, 0, 176, 132, bg, 0, 0, 176);
        displaylcd(33, 55, 110, 20, framebuf, 0, 0, 110);
        progressbar_init(&progressbar, 10, 165, 74, 83, 0x77ff, 0xe8, 0x125f, 0, 100);
        if (fat32_resize_fulldisk(&progressbar))
            panic(PANIC_KILLTHREAD, "Data flash I/O error!");
    }

    ui->blenda(110, 20, 255, framebuf, 0, 0, 110, bg, 33, 55, 176, actions, 0, 20, 110);
    displaylcd(0, 0, 176, 132, bg, 0, 0, 176);
    displaylcd(33, 55, 110, 20, framebuf, 0, 0, 110);
    backlight_set_fade(32);
    backlight_set_brightness(177);
    backlight_on(true);

    disk_mount(0);
    int updating = !(appleflash || rc);
    int cost;
    if (updating)
    {
        cost = commoncost;
        script = commonscript;
    }
    else
    {
        cost = firstinstcost + commoncost;
        script = firstinstscript;
    }
    progressbar_init(&progressbar, 10, 165, 74, 83, 0x77ff, 0xe8, 0x125f, 0, cost);
    sp = 0;
    cost = 0;
    while (script[sp])
    {
        int fd;
        void* data;
        switch (script[sp])
        {
            case 1:
                mkdir((char*)(script[sp + 1]));
                sp += 2;
                break;
            case 2:
                if (script[sp + 2] == 0xffffffff)
                {
                    data = ososptr;
                    script[sp + 3] = osossize;
                }
                else if (script[sp + 2] == 0xfffffffe && appleflash)
                {
                    data = nor;
                    script[sp + 3] = 0x100000;
                }
                else if (script[sp + 2] == 0xfffffffe) script[sp + 3] = 0;
                if (!script[sp + 3])
                {
                    sp += 4;
                    break;
                }
            case 3:
                fd = file_open((char*)(script[sp + 1]), O_RDONLY);
                if (fd >= 0)
                {
                    close(fd);
                    sp += 4;
                    break;
                }
            case 4:
                if (script[sp + 2] < 0xfffffffe) data = (void*)(script[sp + 2]);
                fd = file_creat((char*)(script[sp + 1]));
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
        cost += script[sp++];
        progressbar_setpos(&progressbar, cost, false);
    }

    ui->blenda(110, 20, 255, framebuf, 0, 0, 110, bg, 33, 55, 176, actions, 0, 40, 110);
    displaylcd(33, 55, 110, 20, framebuf, 0, 0, 110);
    progressbar_init(&progressbar, 10, 165, 74, 83, 0x77ff, 0xe8, 0x125f, 0, 256);
    for (i = 0; i < 256; i++)
    {
        bootflash_writeraw(&norbuf[i << 12], i << 12, 1 << 12);
        progressbar_setpos(&progressbar, i, false);
    }

    free(norbuf);
    free(framebuf);
    free(actions);
    free(bg);

    release_library(libui);
    release_library(libpng);
    library_unload(libui);
    library_unload(libpng);

    shutdown(false);
    reset();
}
