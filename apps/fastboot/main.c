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
    if (!(clickwheel_get_state() & 0x1f))
    {
        struct emcorelib_header* libboot = get_library(LIBBOOT_IDENTIFIER, LIBBOOT_API_VERSION, LIBSOURCE_BOOTFLASH, "libboot ");
        if (!libboot) panicf(PANIC_KILLTHREAD, "Could not load booting library!");
        struct libboot_api* boot = (struct libboot_api*)libboot->api;
        int fd = file_open("/.rockbox/rockbox.ipod", O_RDONLY);
        if (fd > 0)
        {
            size = filesize(fd);
            if (size > 0)
            {
                void* buf = memalign(0x10, size);
                if (buf)
                {
                    if (read(fd, buf, size) == size)
                        if (!boot->verify_rockbox_checksum(buf, size))
                            firmware = buf;
                    if (!firmware) free(buf);
                }
            }
            close(fd);
        }
        release_library(libboot);
        library_unload(libboot);
    }
    if (firmware)
    {
        shutdown(false);
        execfirmware((void*)0x08000000, firmware, size);
    }
    else
    {
        int size = bootflash_filesize("bootmenu");
        if (size > 0)
        {
            void* buffer = memalign(0x10, size);
            if (buffer)
            {
                if (bootflash_read("bootmenu", buffer, 0, size) == size)
                {
                    if (execimage(buffer, false) != NULL) return;
                }
                else free(buffer);
            }
        }
        panic(PANIC_KILLTHREAD, "Failed to launch boot menu");
    }
}


EMCORE_APP_HEADER("Fastboot launcher", main, 127)
