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


uint16_t lcdbuffer[320 * 240];
#define BMPIDX_SIDEPANE 0
#define BMPIDX_WARNING 1
#define BMPIDX_INSTALLING 2
#define BMPIDX_FORMATTING 3
#define BMPIDX_COPYING 4
#define BMPIDX_FLASHING 5

struct wakeup eventwakeup;
volatile int button;

char mallocbuf[0xea0000] __attribute__((aligned(16)));
tlsf_pool mallocpool;

extern uint32_t _scriptstart;


#define SHA1CONFIG    (*((volatile uint32_t*)(0x38000000)))
#define SHA1RESET     (*((volatile uint32_t*)(0x38000004)))
#define SHA1RESULT      ((volatile uint32_t*)(0x38000020))
#define SHA1DATAIN      ((volatile uint32_t*)(0x38000040))

void sha1(void* data, uint32_t size, void* hash)
{
    int i, space;
    bool done = false;
    uint32_t tmp32[16];
    uint8_t* tmp8 = (uint8_t*)tmp32;
    uint32_t* databuf = (uint32_t*)data;
    uint32_t* hashbuf = (uint32_t*)hash;
    clockgate_enable(0, true);
    SHA1RESET = 1;
    while (SHA1CONFIG & 1) sleep(0);
    SHA1RESET = 0;
    SHA1CONFIG = 0;
    while (!done)
    {
        space = ((uint32_t)databuf) - ((uint32_t)data) - size + 64;
        if (space > 0)
        {
            for (i = 0; i < 16; i++) tmp32[i] = 0;
            if (space <= 64)
            {
                for (i = 0; i < 64 - space; i++) tmp8[i] = ((uint8_t*)databuf)[i];
                tmp8[64 - space] = 0x80;
            }
            if (space >= 8)
            {
                tmp8[0x3b] = (size >> 29) & 0xff;
                tmp8[0x3c] = (size >> 21) & 0xff;
                tmp8[0x3d] = (size >> 13) & 0xff;
                tmp8[0x3e] = (size >> 5) & 0xff;
                tmp8[0x3f] = (size << 3) & 0xff;
                done = true;
            }
            for (i = 0; i < 16; i++) SHA1DATAIN[i] = tmp32[i];
        }
        else for (i = 0; i < 16; i++) SHA1DATAIN[i] = databuf[i];
        databuf += 16;
        SHA1CONFIG |= 2;
        while (SHA1CONFIG & 1) sleep(0);
        SHA1CONFIG |= 8;
    }
    for (i = 0; i < 5; i++) hashbuf[i] = SHA1RESULT[i];
    clockgate_enable(0, false);
}


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

void mkfat32(struct progressbar_state* progressbar)
{
    uint32_t i, j, rc;
    uint32_t rootdirclus = 2;
    uint32_t secperclus = 1;
    uint32_t fatsectors = 1;
    uint32_t oldfatsectors = 0;
    uint32_t clustercount;
    uint32_t reserved = 2;
    struct storage_info storageinfo;
    storage_get_info(0, &storageinfo);
    uint32_t totalsectors = storageinfo.num_sectors;
    disk_unmount(0);
    while (fatsectors != oldfatsectors)
    {
        oldfatsectors = fatsectors;
        clustercount = (totalsectors - fatsectors - reserved) / secperclus;
        fatsectors = (clustercount + 1025) >> 10;
    }
    uint32_t database = fatsectors + reserved;
    uint32_t clusoffset = 0;
    uint32_t* buf = memalign(0x20000, 0x10);
    memset(buf, 0, 0x800);
    memcpy(buf, "\xeb\x58\x00MSWIN5.0\0\x10", 0xd);
    ((uint8_t*)buf)[0xd] = secperclus;
    ((uint16_t*)buf)[7] = reserved;
    memcpy(&((uint8_t*)buf)[0x10], "\x01\0\0\0\0\xf8\0\0\x3f\0\xff", 0xb);
    buf[8] = totalsectors;
    buf[9] = fatsectors;
    buf[0xb] = rootdirclus + clusoffset;
    ((uint16_t*)buf)[0x18] = 1;
    ((uint8_t*)buf)[0x40] = 0x80;
    ((uint8_t*)buf)[0x42] = 0x29;
    memcpy(&((uint8_t*)buf)[0x43], "\0\0\0\0iPodClassic", 0xf);
    memcpy(&((uint8_t*)buf)[0x52], "FAT32   ", 8);
    ((uint16_t*)buf)[0xff] = 0xaa55;
    if (rc = storage_write_sectors_md(0, 0, 1, buf))
        panicf(PANIC_KILLTHREAD, "Error writing MBR: %08X", rc);
    memset(buf, 0, 0x800);
    buf[0] = 0x41615252;
    buf[0x79] = 0x61417272;
    buf[0x7a] = clustercount - 1;
    buf[0x7b] = 2;
    buf[0x7f] = 0xaa550000;
    if (rc = storage_write_sectors_md(0, 1, 1, buf))
        panicf(PANIC_KILLTHREAD, "Error writing FSINFO: %08X", rc);
    progressbar_init(progressbar, 5, 189, 65, 80, 0, 0xdefb, 0x1d, 0, fatsectors);
    uint32_t cursect = 0;
    for (i = 0; i < fatsectors; i += 32)
    {
        memset(buf, 0, 0x20000);
        if (!i) memcpy(buf, "\xf8\xff\xff\x0f\xff\xff\xff\xff\xff\xff\xff\x0f", 12);
        if (rc = storage_write_sectors_md(0, reserved + i, MIN(fatsectors - i, 32), buf))
            panicf(PANIC_KILLTHREAD, "Error writing FAT sectors %d-%d: %08X", i, MIN(fatsectors - 1, i + 31), rc);
        progressbar_setpos(progressbar, i, false);
    }
    memset(buf, 0, secperclus * 0x1000);
    memcpy(buf, "iPodClassic\x08", 12);
    if (rc = storage_write_sectors_md(0, database, secperclus, buf))
        panicf(PANIC_KILLTHREAD, "Error writing root directory sectors: %08X", i, rc);
    free(buf);
    disk_mount(0);
}

void main(void)
{
    uint32_t i, j, k, rc;
    void* bitmapdata[6];
    uint32_t bitmapsize[6];
    uint32_t* script;
#define scriptb ((uint8_t*)script)
    uint32_t dummy;
    struct progressbar_state progressbar;
    bool appleflash;
    void* syscfgptr;
    uint8_t* norbuf;
#define norbufword ((uint32_t*)norbuf)
    uint8_t* oldnor;
#define oldnorword ((uint32_t*)oldnor)

    button = 0;
    wakeup_init(&eventwakeup);
    button_register_handler(handler);
    mallocpool = tlsf_create(mallocbuf, sizeof(mallocbuf));

    script = &_scriptstart;
    for (i = 0; i < 6; i++)
    {
        bitmapsize[i] = *script;
        bitmapdata[i] = &script[1];
        script = &script[1 + (bitmapsize[i] >> 2)];
    }

    void* bmpbuffer = malloc(0x25900);
    memset(lcdbuffer, 0xff, 0x25800);
    ucl_decompress(bitmapdata[BMPIDX_SIDEPANE], bitmapsize[BMPIDX_SIDEPANE], bmpbuffer, &dummy);
    renderbmp(&lcdbuffer[195], bmpbuffer, 320);
    bool updating = mkdir("/iLoader") == -4;
    if (!updating)
    {
        ucl_decompress(bitmapdata[BMPIDX_WARNING], bitmapsize[BMPIDX_WARNING], bmpbuffer, &dummy);
        renderbmp(lcdbuffer, bmpbuffer, 320);
        displaylcd(0, 319, 0, 239, lcdbuffer, 0);
    }
    else
    {
        ucl_decompress(bitmapdata[BMPIDX_INSTALLING], bitmapsize[BMPIDX_INSTALLING],
                       bmpbuffer, &dummy);
        renderbmp(lcdbuffer, bmpbuffer, 320);
    }
    backlight_set_fade(32);
    backlight_set_brightness(177);
    backlight_on(true);


    norbuf = memalign(0x100000, 0x10);
    oldnor = memalign(0x100000, 0x10);
    memset(norbuf, 0xff, 0x100000);
    bootflash_readraw(oldnor, 0, 0x100000);
    if (oldnorword[0x400] == 0x53436667) appleflash = false;
    else
    {
        if (oldnorword[0] == 0x53436667) appleflash = true;
        else panic(PANIC_KILLTHREAD, "Boot flash contents are damaged! "
                                     "(No SYSCFG found)\n\nPlease ask for help.\n");
    }
    memcpy(&norbuf[0x1000], &oldnor[appleflash ? 0 : 0x1000], 0x1000);
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
            int offs = 0;
            if (flags & 8)
            {
                offs = 0x800;
                size = ((size + 0xf) & ~0xf) + offs;
            }
            if (flags & 1)
            {
                endptr -= ((size + 0xfff) & ~0xfff);
                memcpy(&norbuf[endptr + offs], data, size);
                file = endptr;
            }
            else
            {
                memcpy(&norbuf[beginptr + offs], data, size);
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
            if (flags & 8)
            {
                size -= offs;
                memset(&norbuf[file], 0, 0x800);
                memcpy(&norbuf[file], "87021.0\x01", 8);
                *((uint32_t*)&norbuf[file + 0xc]) = size;
                sha1(&norbuf[file + 0x800], size, &norbuf[file + 0x10]);
                *((uint32_t*)&norbuf[file + 0x20]) = 0;
                hwkeyaes(HWKEYAES_ENCRYPT, 2, &norbuf[file + 0x10], 0x10);
                sha1(&norbuf[file], 0x40, &norbuf[file + 0x40]);
                *((uint32_t*)&norbuf[file + 0x50]) = 0;
                hwkeyaes(HWKEYAES_ENCRYPT, 2, &norbuf[file + 0x40], 0x10);
                hwkeyaes(HWKEYAES_ENCRYPT, 2, &norbuf[file + 0x800], size);
            }
        }
        else if (!(flags & 4)) sp += 2;
    }

    if (!updating)
    {
        while (true)
        {
            wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
            if (button == 2) break;
            else if (button == 4)
            {
                shutdown(false);
                reset();
            }
            button = 0;
        }
        memset(lcdbuffer, 0xff, 0x25800);
        ucl_decompress(bitmapdata[BMPIDX_SIDEPANE], bitmapsize[BMPIDX_SIDEPANE], bmpbuffer, &dummy);
        renderbmp(&lcdbuffer[195], bmpbuffer, 320);
        ucl_decompress(bitmapdata[BMPIDX_INSTALLING], bitmapsize[BMPIDX_INSTALLING],
                       bmpbuffer, &dummy);
        renderbmp(lcdbuffer, bmpbuffer, 320);
        ucl_decompress(bitmapdata[BMPIDX_FORMATTING], bitmapsize[BMPIDX_FORMATTING],
                        bmpbuffer, &dummy);
        renderbmp(&lcdbuffer[320 * 36], bmpbuffer, 320);
        displaylcd(0, 319, 0, 239, lcdbuffer, 0);
        mkfat32(&progressbar);
    }

    ucl_decompress(bitmapdata[BMPIDX_COPYING], bitmapsize[BMPIDX_COPYING],
                   bmpbuffer, &dummy);
    renderbmp(&lcdbuffer[320 * 72], bmpbuffer, 320);
    displaylcd(0, 319, 0, 239, lcdbuffer, 0);
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
    progressbar_init(&progressbar, 5, 189, 101, 116, 0, 0xdefb, 0x1d, 0, status);
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
                if (script[sp + 2] == 0xfffffffe && appleflash)
                {
                    data = oldnor;
                    script[sp + 3] = 0x100000;
                }
                else if (script[sp + 2] == 0xfffffffe) script[sp + 3] = 0;
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
    ucl_decompress(bitmapdata[BMPIDX_FLASHING], bitmapsize[BMPIDX_FLASHING], bmpbuffer, &dummy);
    renderbmp(&lcdbuffer[320 * 108], bmpbuffer, 320);
    displaylcd(0, 319, 0, 239, lcdbuffer, 0);
    progressbar_init(&progressbar, 5, 189, 162, 178, 0, 0xdefb, 0x1d, 0, 256);
    for (i = 0; i < 256; i++)
    {
        bootflash_writeraw(&norbuf[i << 12], i << 12, 1 << 12);
        progressbar_setpos(&progressbar, i, false);
    }

    shutdown(false);
    reset();
}
