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
#endif


static const char welcomestring[] INITCONST_ATTR = "emBIOS v" VERSION " r" VERSION_SVN "\n\n";
static const char initthreadname[] INITCONST_ATTR = "Initialisation thread";
static uint32_t initstack[0x400] INITBSS_ATTR;


void initthread() INITCODE_ATTR;
void initthread()
{
    cputs(CONSOLE_BOOT, welcomestring);
	#ifdef HAVE_I2C
    i2c_init();
	#endif
    power_init();
	#ifdef HAVE_USB
    usb_init();
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
    DEBUGF("Finished initialisation sequence");
}

void init() INITCODE_ATTR;
void init()
{
    scheduler_init();
    console_init();
	#ifdef HAVE_LCD
    lcd_init();
    lcdconsole_init();
	#endif
    interrupt_init();
    thread_create(initthreadname, initthread, initstack,
                  sizeof(initstack), USER_THREAD, 127, true);
}
