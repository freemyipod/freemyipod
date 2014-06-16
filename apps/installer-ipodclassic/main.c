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
#include "libmkfat32.h"


static void main(int argc, const char** argv);
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
extern char f_mkfat32_emcorelib[];
extern uint32_t flashscript[];
extern uint32_t firstinstcost;
extern uint32_t firstinstscript[];
extern uint32_t commoncost;
extern uint32_t commonscript[];


static struct wakeup eventwakeup;
static volatile int button;
static volatile int scrollpos;


#define SHA1CONFIG    (*((volatile uint32_t*)(0x38000000)))
#define SHA1RESET     (*((volatile uint32_t*)(0x38000004)))
#define SHA1RESULT      ((volatile uint32_t*)(0x38000020))
#define SHA1DATAIN      ((volatile uint32_t*)(0x38000040))

static void sha1(void* data, uint32_t size, void* hash)
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
        else for (i = 0; i < 16; i++) SHA1DATAIN[i] = *databuf++;
        SHA1CONFIG |= 2;
        while (SHA1CONFIG & 1) sleep(0);
        SHA1CONFIG |= 8;
    }
    for (i = 0; i < 5; i++) *hashbuf++ = SHA1RESULT[i];
    clockgate_enable(0, false);
}


static void handler(void* user, enum button_event eventtype, int which, int value)
{
    if (eventtype == BUTTON_PRESS) button |= 1 << which;
    if (eventtype == BUTTON_RELEASE) button &= ~(1 << which);
    if (eventtype == WHEEL_MOVED_ACCEL)
        scrollpos = MAX(0, MIN(295, scrollpos + value / 8));
    wakeup_signal(&eventwakeup);
}

static void fat32_progressbar_init(void* user, int max)
{
    progressbar_init((struct progressbar_state*)user,
                     15, 304, 135, 159, 0x77ff, 0xe8, 0x125f, 0, max);
}

static void fat32_progressbar_update(void* user, int current)
{
    progressbar_setpos((struct progressbar_state*)user, current, false);
}

static void main(int argc, const char** argv)
{
    uint32_t i, j, k, rc;
    uint32_t dummy;
    struct progressbar_state progressbar;
    bool appleflash = false;
    bool oldflash;
    void* syscfgptr;
    uint8_t* norbuf;
#define norbufword ((uint32_t*)norbuf)
    uint8_t* oldnor;
#define oldnorword ((uint32_t*)oldnor)

    cputc(3, '.');
    struct emcorelib_header* libpng = get_library(LIBPNG_IDENTIFIER, LIBPNG_API_VERSION, LIBSOURCE_RAM_NEEDCOPY, f_png_emcorelib);
    if (!libpng) panicf(PANIC_KILLTHREAD, "Could not load PNG decoder library!");
    struct libpng_api* png = (struct libpng_api*)libpng->api;
    cputc(3, '.');
    struct emcorelib_header* libui = get_library(LIBUI_IDENTIFIER, LIBUI_API_VERSION, LIBSOURCE_RAM_NEEDCOPY, f_ui_emcorelib);
    if (!libui) panicf(PANIC_KILLTHREAD, "Could not load user interface library!");
    struct libui_api* ui = (struct libui_api*)libui->api;
    cputc(3, '.');
    struct emcorelib_header* libmkfat32 = get_library(LIBMKFAT32_IDENTIFIER, LIBMKFAT32_API_VERSION, LIBSOURCE_RAM_NEEDCOPY, f_mkfat32_emcorelib);
    if (!libmkfat32) panicf(PANIC_KILLTHREAD, "Could not load mkfat32 library!");
    struct libmkfat32_api* mf32 = (struct libmkfat32_api*)libmkfat32->api;
    cputc(3, '.');

    struct png_info* handle = png->png_open(background_png, background_png_size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse background image!");
    cputc(3, '.');
    struct png_rgb* bg = png->png_decode_rgb(handle);
    if (!bg) panicf(PANIC_KILLTHREAD, "Could not decode background image!");
    png->png_destroy(handle);
    cputc(3, '.');
    handle = png->png_open(actions_png, actions_png_size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse actions image!");
    cputc(3, '.');
    struct png_rgba* actions = png->png_decode_rgba(handle);
    if (!actions) panicf(PANIC_KILLTHREAD, "Could not decode actions image!");
    png->png_destroy(handle);
    cputc(3, '.');
    void* framebuf = malloc(290 * 165 * 3);
    if (!framebuf) panicf(PANIC_KILLTHREAD, "Could not allocate frame buffer!");
    cputc(3, '.');

    disk_unmount(0);
    bool updating = disk_mount(0);
    cputc(3, '.');

    norbuf = memalign(0x10, 0x100000);
    oldnor = memalign(0x10, 0x100000);
    memset(norbuf, 0xff, 0x100000);
    cputc(3, '.');
    bootflash_readraw(oldnor, 0, 0x100000);
    cputc(3, '.');
    if (oldnorword[0x400] == 0x53436667) oldflash = true;
    else
    {
        if (oldnorword[0] == 0x53436667) oldflash = false;
        else panic(PANIC_KILLTHREAD, "Boot flash contents are damaged! "
                                     "(No SYSCFG found)\n\nPlease ask for help.\n");
        if (oldnorword[0x400] == 0xffffffff && oldnorword[0x3ff80] == 0x666c7368)
        {
            appleflash = true;
            updating = false;
        }
    }
    memcpy(norbuf, &oldnor[oldflash ? 0x1000 : 0], 0x1000);
    cputc(3, '.');

    uint32_t* script = flashscript;
    uint32_t sp = 0;
    uint32_t beginptr = 0x2000;
    uint32_t endptr = 0x100000;
    uint32_t dirptr = 0x1000;
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
            int offs = 0;
            if (flags & 8)
            {
                offs = 0x800;
                size = ((size + 0xf) & ~0xf);
            }
            if (flags & 1)
            {
                endptr -= ((offs + size + 0xfff) & ~0xfff);
                file = endptr;
            }
            else
            {
                file = beginptr;
                beginptr += ((offs + size + 0xfff) & ~0xfff);
            }
            memcpy(&norbuf[file + offs], data, size);
            if (!(flags & 4))
            {
                if (dirptr >= 0x2000)
                    panicf(PANIC_KILLTHREAD, "Error: Directory is full!");
                memcpy(&norbuf[dirptr], &script[sp], 8);
                norbufword[(dirptr >> 2) + 2] = file;
                norbufword[(dirptr >> 2) + 3] = storesize;
                dirptr += 0x10;
                sp += 2;
            }
            if (flags & 8)
            {
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
        cputc(3, '.');
    }

    if (!updating)
    {
        void* darkened = malloc(320 * 240 * 3);
        if (!darkened) panicf(PANIC_KILLTHREAD, "Could not allocate darkened image!");
        cputc(3, '.');
        handle = png->png_open(darkener_png, darkener_png_size);
        if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse darkener image!");
        cputc(3, '.');
        struct png_rgba* darkener = png->png_decode_rgba(handle);
        if (!darkener) panicf(PANIC_KILLTHREAD, "Could not decode darkener image!");
        png->png_destroy(handle);
        cputc(3, '.');
        ui->blenda(320, 240, 255, darkened, 0, 0, 320, bg, 0, 0, 320, darkener, 0, 0, 320);
        free(darkener);
        cputc(3, '.');
        handle = png->png_open(disclaimer_png, disclaimer_png_size);
        if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse disclaimer image!");
        cputc(3, '.');
        struct png_rgba* disclaimer = png->png_decode_rgba(handle);
        if (!disclaimer) panicf(PANIC_KILLTHREAD, "Could not decode disclaimer image!");
        png->png_destroy(handle);
        cputc(3, '.');

        button = 0;
        wakeup_init(&eventwakeup);
        struct button_hook_entry* hook = button_register_handler(handler, NULL);
        if (!hook) panicf(PANIC_KILLTHREAD, "Could not register button hook!");

        displaylcd(0, 0, 320, 240, darkened, 0, 0, 320);
        backlight_set_fade(32);
        backlight_set_brightness(177);
        backlight_on(true);
        scrollpos = 0;

        while (true)
        {
            ui->blenda(290, 165, 255, framebuf, 0, 0, 290,
                       darkened, 15, 50, 320, disclaimer, 0, scrollpos, 290);
            displaylcd(15, 50, 290, 165, framebuf, 0, 0, 290);

            wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
            if (button == 0x18) break;
            else if (button == 4)
            {
                shutdown(false);
                reset();
            }
        }

        button_unregister_handler(hook);
        free(darkened);
        free(disclaimer);
                
        ui->blenda(165, 36, 255, framebuf, 0, 0, 165, bg, 77, 100, 320, actions, 0, 0, 165);
        displaylcd(0, 0, 320, 240, bg, 0, 0, 320);
        displaylcd(77, 100, 165, 36, framebuf, 0, 0, 165);

        struct storage_info storageinfo;
        storage_get_info(0, &storageinfo);
        int rc = mf32->mkfat32(0, 0, storageinfo.num_sectors, 4096, 1, "iPodClassic", &progressbar,
                              fat32_progressbar_init, fat32_progressbar_update);
        if (rc < 0) panicf(PANIC_KILLTHREAD, "Error formatting hard drive: %08X", rc);
    }

    ui->blenda(165, 36, 255, framebuf, 0, 0, 165, bg, 77, 100, 320, actions, 0, 36, 165);
    displaylcd(0, 0, 320, 240, bg, 0, 0, 320);
    displaylcd(77, 100, 165, 36, framebuf, 0, 0, 165);
    backlight_set_fade(32);
    backlight_set_brightness(177);
    backlight_on(true);

    remove("/.boot/init.emcoreapp");
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
    progressbar_init(&progressbar, 15, 304, 135, 159, 0x77ff, 0xe8, 0x125f, 0, cost);
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

    if (oldnor) free(oldnor);

    ui->blenda(165, 36, 255, framebuf, 0, 0, 165, bg, 77, 100, 320, actions, 0, 72, 165);
    displaylcd(77, 100, 165, 36, framebuf, 0, 0, 165);
    progressbar_init(&progressbar, 15, 304, 135, 159, 0x77ff, 0xe8, 0x125f, 0, 256);
    for (i = 0; i < 256; i++)
    {
        bootflash_writeraw(&norbuf[i << 12], i << 12, 1 << 12);
        progressbar_setpos(&progressbar, i, false);
    }

    free(norbuf);
    free(framebuf);
    free(actions);
    free(bg);

    release_library(libmkfat32);
    release_library(libui);
    release_library(libpng);
    library_unload(libmkfat32);
    library_unload(libui);
    library_unload(libpng);

    shutdown(false);
    reset();
}
