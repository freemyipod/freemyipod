//
//
//    Copyright 2011 TheSeven
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
#include "resources.h"
#include "main.h"
#include "boot.h"
#include "settings.h"
#include "mainchooser.h"
#include "settingchooser.h"
#include "util.h"


struct libboot_api* boot;
struct libui_api* ui;
void* framebuf;
void* framebuf2;
void* bg;
void* icons;
void* rbxlogo;
struct bootinfo_t bootinfo =
{
    .valid = false,
    .firmware = NULL,
    .size = 0,
    .app = NULL,
    .argc = 0,
    .argv = NULL
};


static void main(int argc, const char** argv)
{
    settings_init();
    
    struct emcorelib_header* libboot = loadlib(LIBBOOT_IDENTIFIER,
                                               LIBBOOT_API_VERSION, "libboot ");
    boot = (struct libboot_api*)libboot->api;

    if (!(clickwheel_get_state() & 0x1f))
        switch (settings.fastboot_item)
        {
            case 1:
                fastboot_rockbox();
                break;
            
            case 2:
                fastboot_diskmode();
                break;
            
            case 3:
                fastboot_umsboot();
                break;
            
            case 4:
                bootinfo.valid = true;
                break;
        }

    if (!bootinfo.valid)
    {
        struct emcorelib_header* libpng = loadlib(LIBPNG_IDENTIFIER, LIBPNG_API_VERSION, "libpng  ");
        struct libpng_api* png = (struct libpng_api*)libpng->api;
        bg = loadpng(png, background_png, background_png_size, (void* (*)(struct png_info*))(png->png_decode_rgb));
        icons = loadpng(png, icons_png, icons_png_size, (void* (*)(struct png_info*))(png->png_decode_rgba));
        rbxlogo = loadpng(png, rockbox_png, rockbox_png_size, (void* (*)(struct png_info*))(png->png_decode_rgb));
        release_library(libpng);
        library_unload(libpng);
        
        struct emcorelib_header* libui = loadlib(LIBUI_IDENTIFIER, LIBUI_API_VERSION, "libui   ");
        ui = (struct libui_api*)libui->api;
        
        framebuf = malloc(320 * 240 * 3);
        if (!framebuf) panicf(PANIC_KILLTHREAD, "Could not allocate framebuffer!");
        framebuf2 = malloc(320 * 240 * 3);
        if (!framebuf2) panicf(PANIC_KILLTHREAD, "Could not allocate framebuffer 2!");

        mainchooser_init();
        toolchooser_init();
        settingchooser_init();
        confirmchooser_init();
        snow_init();
        
        run_mainchooser();
        
        free(framebuf2);
        free(framebuf);
        free(rbxlogo);
        free(icons);
        free(bg);
        release_library(libui);
        library_unload(libui);
    }

    release_library(libboot);
    library_unload(libboot);

    if (bootinfo.firmware)
    {
        shutdown(false);
        execfirmware((void*)0x08000000, bootinfo.firmware, bootinfo.size);
    }
    else if (bootinfo.app) execimage(bootinfo.app, false, bootinfo.argc, bootinfo.argv);
    else cputs(3, "Dropped into emCORE console.\n");
}

EMCORE_APP_HEADER("Boot menu", main, 127)
