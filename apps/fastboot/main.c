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
#include "libboot.h"


static void main()
{
    void* firmware = NULL;
    int size;
    struct emcorelib_header* lib = get_library(LIBBOOT_IDENTIFIER, LIBBOOT_API_VERSION, LIBSOURCE_BOOTFLASH, "libboot ");
    if (!lib) panicf(PANIC_KILLTHREAD, "Could not load libboot");
    struct libboot_api* boot = (struct libboot_api*)lib->api;
    if (!(clickwheel_get_state() & 0x1f))
        boot->load_from_file(&firmware, &size, true, "/.rockbox/rockbox.ipod", 0);
    if (firmware)
    {
        release_library(lib);
        library_unload(lib);
        shutdown(false);
        execfirmware((void*)0x08000000, firmware, size);
    }
    else
    {
        boot->load_from_flash(&firmware, &size, false, "bootmenu", 0);
        if (!firmware || execimage(firmware, false) == NULL)
            panic(PANIC_KILLTHREAD, "Failed to launch boot menu");
    }
}


EMCORE_APP_HEADER("Fastboot launcher", main, 127)
