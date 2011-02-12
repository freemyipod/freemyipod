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


#include "global.h"
#include "thread.h"
#include "console.h"
#include "power.h"
#include "interrupt.h"
#include "ucl.h"
#include "util.h"
#include "execimage.h"
#include "targetinit.h"
#include "malloc.h"
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


extern int _poolstart;   // Not an int at all, but gcc complains about void types being
                         // used here, and we only need the address, so just make it happy...


enum boottype
{
    BOOTTYPE_PIGGYBACKED = 1,
    BOOTTYPE_BOOTFLASH = 2,
    BOOTTYPE_FILESYSTEM = 3,
    BOOTTYPE_FAKESUCCESS = 4
};

struct bootoption
{
    struct bootoption* success_next;
    struct bootoption* fail_next;
    int type;
    char* source;
};

struct bootinfo
{
    char signature[8];
    int version;
    void* baseaddr;
    size_t totalsize;
    struct bootoption* options;
};


struct initbss
{
    struct scheduler_thread initthread;
    uint32_t initstack[0x400];
    void* bootalloc;
#ifdef HAVE_STORAGE
    struct scheduler_thread storagethread;
    uint32_t storagestack[0x400];
    struct wakeup storagewakeup;
#endif
};


static struct initbss* ib INITDATA_ATTR = NULL;
static const char welcomestring[] INITCONST_ATTR = "emCORE v" VERSION " r" VERSION_SVN "\n\n";
static const char initthreadname[] INITCONST_ATTR = "Initialization thread";
static const char unknownboottypestr[] INITCONST_ATTR = "Skipping boot option with unknown type %d\n";
static const char nobootoptionsstr[] INITCONST_ATTR = "No usable boot options, waiting for USB commands\n\n";
#ifdef HAVE_STORAGE
static const char storagethreadname[] INITCONST_ATTR = "Storage init thread";
#endif

struct bootinfo bootinfo INITTAIL_ATTR =
{
    .signature = "emCOboot",
    .version = 1,
    .baseaddr = &bootinfo,
    .totalsize = sizeof(struct bootinfo),
    .options = NULL
};


#ifdef HAVE_STORAGE
void storageinitthread() INITCODE_ATTR;
void storageinitthread()
{
    DEBUGF("Initializing storage drivers...");
    storage_init();
    DEBUGF("Initializing storage subsystem...");
    disk_init_subsystem();
    DEBUGF("Reading partition tables...");
    disk_init();
    DEBUGF("Mounting partitions...");
    disk_mount_all();
    DEBUGF("Storage init finished.");
    wakeup_signal(&(ib->storagewakeup));
}
#endif

void initthread() INITCODE_ATTR;
void initthread()
{
#ifdef HAVE_I2C
    i2c_init();
#endif
    power_init();
    cputs(CONSOLE_BOOT, welcomestring);
#ifdef HAVE_STORAGE
    wakeup_init(&(ib->storagewakeup));
    thread_create(&(ib->storagethread), storagethreadname, storageinitthread,
                  ib->storagestack, sizeof(ib->storagestack), USER_THREAD, 127, true);
#endif
#ifdef HAVE_USB
    usb_init();
#endif
#ifdef HAVE_BACKLIGHT
    backlight_init();
#endif
#ifdef HAVE_BUTTON
    button_init();
#endif
#ifdef HAVE_TARGETINIT_LATE
    targetinit_late();
#endif
#ifdef HAVE_STORAGE
    while (true)
    {
        if (wakeup_wait(&(ib->storagewakeup), 100000) == THREAD_OK) break;
        enum thread_state state = thread_get_state(&(ib->storagethread));
        if (state == THREAD_DEFUNCT_ACK)
        {
            if (wakeup_wait(&(ib->storagewakeup), 0) == THREAD_OK) break;
            thread_terminate(&(ib->storagethread));
            break;
        }
    }
#endif
#ifdef HAVE_TARGETINIT_VERYLATE
    targetinit_verylate();
#endif
    DEBUGF("Finished initialisation sequence");

    struct bootoption* option = bootinfo.options;
    bool success = false;
    while (option)
    {
        success = false;
        switch (option->type)
        {
        case BOOTTYPE_PIGGYBACKED:
            success = execimage(option->source, true) != NULL;
            break;

#ifdef HAVE_BOOTFLASH
        case BOOTTYPE_BOOTFLASH:
        {
            int size = bootflash_filesize(option->source);
            if (size <= 0) break;
            void* buffer = memalign(0x10, size);
            if (!buffer) break;
            if (bootflash_read(option->source, buffer, 0, size) != size)
            {
                free(buffer);
                break;
            }
            success = execimage(buffer, false) != NULL;
            break;
        }
#endif

#ifdef HAVE_STORAGE
        case BOOTTYPE_FILESYSTEM:
        {
            int fd = file_open(option->source, O_RDONLY);
            if (fd <= 0) break;
            int size = filesize(fd);
            if (size <= 0)
            {
                close(fd);
                break;
            }
            void* buffer = memalign(0x10, size);
            if (!buffer)
            {
                close(fd);
                break;
            }
            if (read(fd, buffer, size) != size)
            {
                free(buffer);
                close(fd);
                break;
            }
            close(fd);
            success = execimage(buffer, false) != NULL;
            break;
        }
#endif

        case BOOTTYPE_FAKESUCCESS:
            success = true;
            break;

        default:
            cprintf(CONSOLE_BOOT, unknownboottypestr, option->type);
        }
        if (success) option = option->success_next;
        else option = option->fail_next;
    }
    if (!success) cputs(CONSOLE_BOOT, nobootoptionsstr);
    free(ib->bootalloc);
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
    malloc_init();
    size_t size = (size_t)(&bootinfo) - (size_t)(&_poolstart) + bootinfo.totalsize;
    void* bootalloc = malloc(size);
    size -= (size_t)(bootalloc) - (size_t)(&_poolstart);
    realloc(bootalloc, size);
    ib = (struct initbss*)malloc(sizeof(struct initbss));
    reownalloc(ib, &(ib->initthread));
    reownalloc(bootalloc, &(ib->initthread));
    ib->bootalloc = bootalloc;
    thread_create(&(ib->initthread), initthreadname, initthread, ib->initstack,
                  sizeof(ib->initstack), OS_THREAD, 127, true);
    timer_init();
    interrupt_init();
}
