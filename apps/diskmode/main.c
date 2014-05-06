//
//
//    Copyright 2013 TheSeven
//    Copyright 2014 user890104
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
#include "ums.h"
#include "usb.h"


static void main(int argc, const char** argv)
{
    cputs(1, "Welcome to Disk Mode!\n\n"
             "This device will now behave\nlike an external hard drive.\n\n"
             "When you're finished, please\nproperly eject/unmount the disk.\n");
             
    ums_init();
    usb_prepare();
    
    disk_unmount(0);
    usb_connect();
    
    cprintf(3, "Disk was ejected successfully.\nReturning to the bootmenu...\n\n");
    
    struct emcorelib_header* libboot = get_library(LIBBOOT_IDENTIFIER, LIBBOOT_API_VERSION,
                                                   LIBSOURCE_BOOTFLASH, "libboot ");
    if (!libboot) panicf(PANIC_KILLTHREAD, "Could not load libboot");
    struct libboot_api* boot = (struct libboot_api*)libboot->api;
    
    int size = 0;
    void* app = NULL;
    boot->load_from_flash(&app, &size, false, "bootmenu", 0);
    if (!app)
        panicf(PANIC_KILLTHREAD, "Unable to start the bootmenu! Press MENU+SELECT to reboot your device.\n");
        
    release_library(libboot);
    
    disk_mount(0);
    execimage(app, false, 0, NULL);
}


EMCORE_APP_HEADER("Disk Mode", main, 127)

