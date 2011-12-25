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
#include "libmkfat32.h"
#include "tools.h"
#include "util.h"
#include "main.h"
#include "confirmchooser.h"


void run_clearcfg()
{
    if (!run_confirmchooser("Really clear Rockbox config?")) return;
    remove("/.rockbox/config.cfg");
    memcpy(framebuf, bg, 320 * 240 * 3);
    message(97, "Rockbox configuration", "  has been cleared.  ");
}

void run_cleardb()
{
    if (!run_confirmchooser("Really clear Rockbox database?")) return;
    remove("/.rockbox/database_0.tcd");
    remove("/.rockbox/database_1.tcd");
    remove("/.rockbox/database_2.tcd");
    remove("/.rockbox/database_3.tcd");
    remove("/.rockbox/database_4.tcd");
    remove("/.rockbox/database_5.tcd");
    remove("/.rockbox/database_6.tcd");
    remove("/.rockbox/database_7.tcd");
    remove("/.rockbox/database_8.tcd");
    remove("/.rockbox/database_9.tcd");
    remove("/.rockbox/database_idx.tcd");
    remove("/.rockbox/database_tmp.tcd");
    remove("/.rockbox/database_state.tcd");
    remove("/.rockbox/database_changelog.txt");
    memcpy(framebuf, bg, 320 * 240 * 3);
    message(109, "Rockbox  database", "has been cleared.");
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

void run_reformat()
{
    if (!run_confirmchooser("Really reformat data partition?")) return;
    memcpy(framebuf, bg, 320 * 240 * 3);
    rendertext(framebuf, 70, 125, 320, 0xff7fffff, 0xa0000000, "Reformatting data partition...");
    update_display(NULL);
    displaylcd(0, 0, 320, 240, framebuf, 0, 0, 320);
    struct emcorelib_header* libmkfat32 = loadlib(LIBMKFAT32_IDENTIFIER,
                                                  LIBMKFAT32_API_VERSION, "libmkf32");
    struct libmkfat32_api* mf32 = (struct libmkfat32_api*)libmkfat32->api;
    struct storage_info storageinfo;
    storage_get_info(0, &storageinfo);
    struct progressbar_state progressbar;
    int rc = mf32->mkfat32(0, 0, storageinfo.num_sectors, 4096, 1, "iPodClassic",
                           &progressbar, fat32_progressbar_init, fat32_progressbar_update);
    if (rc < 0) panicf(PANIC_KILLTHREAD, "Error formatting hard drive: %08X", rc);
    release_library(libmkfat32);
    library_unload(libmkfat32);
    memcpy(framebuf, bg, 320 * 240 * 3);
    message(106, "Data partition has", "been  reformatted.");
}
