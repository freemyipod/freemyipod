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


#include "global.h"
#include "thread.h"
#include "console.h"
#include "power.h"
#include "interrupt.h"
#include "ucl.h"
#include "util.h"
#include "execimage.h"
#include "targetinit.h"
#ifdef HAVE_LCD
#include "lcd.h"
#include "lcdconsole.h"
#endif
#ifdef HAVE_I2C
#include "i2c.h"
#endif
#ifdef HAVE_USB
#include "usb/usb.h"
#endif
#ifdef HAVE_STORAGE
#include "storage.h"
#include "disk.h"
#include "file.h"
#endif
#ifdef HAVE_BOOTFLASH
#include "bootflash.h"
#endif
#ifdef HAVE_BACKLIGHT
#include "backlight.h"
#endif


struct bootinfo_t
{
    char signature[8];
    int version;
    bool trydataflash;
    char dataflashpath[256];
    bool dataflashflags;
    void* dataflashdest;
    bool trybootflash;
    char bootimagename[8];
    bool bootflashflags;
    void* bootflashdest;
    bool trymemmapped;
    void* memmappedaddr;
    int memmappedsize;
    bool memmappedflags;
    void* memmappeddest;
};


static const char welcomestring[] INITCONST_ATTR = "emBIOS v" VERSION " r" VERSION_SVN "\n\n";
static const char initthreadname[] INITCONST_ATTR = "Initialization thread";
static uint32_t initstack[0x400] INITSTACK_ATTR;
extern int _loadspaceend;
struct bootinfo_t bootinfo_src INITHEAD_ATTR =
{
    .signature = "emBIboot",
    .version = 0,
    .trydataflash = false,
    .trybootflash = false,
    .trymemmapped = false
};


void boot()
{
    struct bootinfo_t bootinfo = bootinfo_src;
#ifdef HAVE_STORAGE
    if (bootinfo.trydataflash)
    {
        int fd = file_open(bootinfo.dataflashpath, O_RDONLY);
        if (fd < 0) goto dataflashfailed;
        int size = filesize(fd);
        if (size < 0) goto dataflashfailed;
        if (bootinfo.dataflashflags & 1)
        {
            void* addr = (void*)((((uint32_t)&_loadspaceend) - size) & ~(CACHEALIGN_SIZE - 1));
            if (read(fd, addr, size) != size) goto dataflashfailed;
            if (ucl_decompress(addr, size, bootinfo.dataflashdest, (uint32_t*)&size))
                goto dataflashfailed;
        }
        else if (read(fd, bootinfo.dataflashdest, size) != size) goto dataflashfailed;
        if (execimage(bootinfo.dataflashdest) >= 0) return;
    }
dataflashfailed:
#endif
#ifdef HAVE_BOOTFLASH
    if (bootinfo.trybootflash)
    {
        int size = bootflash_filesize(bootinfo.bootimagename);
        if (size < 0) goto bootflashfailed;
#ifdef BOOTFLASH_IS_MEMMAPPED
        void* addr = bootflash_getaddr(bootinfo.bootimagename);
        if (!addr) goto bootflashfailed;
        if (bootinfo.bootflashflags & 1)
        {
            if (ucl_decompress(addr, size, bootinfo.bootflashdest, (uint32_t*)&size))
                goto bootflashfailed;
            if (execimage(bootinfo.bootflashdest) >= 0) return;
        }
        else if (bootinfo.bootflashflags & 2)
        {
            memcpy(bootinfo.bootflashdest, addr, size);
            if (execimage(bootinfo.bootflashdest) >= 0) return;
        }
        else if (execimage(addr) >= 0) return;
#else
        if (bootinfo.bootflashflags & 1)
        {
            void* addr = (void*)((((uint32_t)&_loadspaceend) - size) & ~(CACHEALIGN_SIZE - 1));
            if (bootflash_read(bootinfo.bootimagename, addr, 0, size) != size)
                goto bootflashfailed;
            if (ucl_decompress(addr, size, bootinfo.bootflashdest, (uint32_t*)&size))
                goto bootflashfailed;
        }
        else if (bootflash_read(bootinfo.bootimagename, bootinfo.bootflashdest, 0, size) != size)
            goto bootflashfailed;
        if (execimage(bootinfo.bootflashdest) >= 0) return;
#endif
    }
bootflashfailed:
#endif
    if (bootinfo.trymemmapped)
    {
        int size = bootinfo.memmappedsize;
        if (bootinfo.memmappedflags & 1)
        {
            if (ucl_decompress(bootinfo.memmappedaddr, size,
                               bootinfo.memmappeddest, (uint32_t*)&size))
                goto memmappedfailed;
            if (execimage(bootinfo.memmappeddest) >= 0) return;
        }
        else if (bootinfo.memmappedflags & 2)
        {
            memcpy(bootinfo.memmappeddest, bootinfo.memmappedaddr, size);
            if (execimage(bootinfo.memmappeddest) >= 0) return;
        }
        else if (execimage(bootinfo.memmappedaddr) >= 0) return;
    }
memmappedfailed:
    if (bootinfo.trydataflash || bootinfo.trybootflash || bootinfo.trymemmapped)
        cputs(CONSOLE_BOOT, "Could not find a usable boot image!\n");
    cputs(CONSOLE_BOOT, "Waiting for USB commands\n\n");
}

void initthread() INITCODE_ATTR;
void initthread()
{
#ifdef HAVE_I2C
    i2c_init();
#endif
    power_init();
#ifdef HAVE_USB
    usb_init();
#endif
    cputs(CONSOLE_BOOT, welcomestring);
#ifdef HAVE_BACKLIGHT
    backlight_init();
#endif
#ifdef HAVE_BUTTON
    button_init();
#endif
#ifdef HAVE_STORAGE
    DEBUGF("Initializing storage drivers...");
    storage_init();
    DEBUGF("Initializing storage subsystem...");
    disk_init_subsystem();
    DEBUGF("Reading partition tables...");
    disk_init();
    DEBUGF("Mounting partitions...");
    disk_mount_all();
#endif
#ifdef HAVE_TARGETINIT_LATE
    targetinit_late();
#endif
    DEBUGF("Finished initialisation sequence");
    boot();
}

void init() INITCODE_ATTR;
void init()
{
#ifdef HAVE_TARGETINIT_VERYEARLY
    targetinit_veryearly();
#endif
    scheduler_init();
    console_init();
#ifdef HAVE_LCD
    lcd_init();
    lcdconsole_init();
#endif
#ifdef HAVE_TARGETINIT_EARLY
    targetinit_early();
#endif
    interrupt_init();
    thread_create(initthreadname, initthread, initstack,
                  sizeof(initstack), USER_THREAD, 127, true);
}
