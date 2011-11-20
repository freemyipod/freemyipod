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
#include "boot.h"
#include "main.h"


void run_powerdown(void** firmware, void** app, int* size)
{
    shutdown(true);
    power_off();
}

void fastboot_rockbox(void** firmware, void** app, int* size)
{
    boot->load_from_file(firmware, size, true, "/.rockbox/rockbox.ipod", 0);
}

void run_rockbox_fallback(void** firmware, void** app, int* size)
{
    boot->load_from_flash(firmware, size, true, "rockbox ", 0x100000);
    if (!*firmware)
    {
        memcpy(framebuf, bg, 320 * 240 * 3);
        message(91, "Loading Rockbox failed!", "Returning to main menu.");
    }
}

void run_rockbox(void** firmware, void** app, int* size)
{
    int i;
    for (i = 1; i <= 96; i += 19)
    {
        if (i < 96)
            ui->blend(320, 240, 50, framebuf, 0, 0, 320,
                        framebuf, 0, 0, 320, bg, 0, 0, 320);
        else memcpy(framebuf, bg, 320 * 240 * 3);
        ui->blit(280, MIN(86, i), 3, framebuf, 20, MAX(0, i - 86), 320,
                    rbxlogo, 0, MAX(0, 86 - i), 280);
        displaylcd(0, 0, 320, 240, framebuf, 0, 0, 320);
    }
    fastboot_rockbox(firmware, app, size);
    if (!*firmware)
    {
        message(76, "Loading rockbox.ipod failed!", "  Trying fallback image...  ");
        run_rockbox_fallback(firmware, app, size);
    }
}

void fastboot_umsboot(void** firmware, void** app, int* size)
{
    boot->load_from_flash(firmware, size, false, "umsboot ", 0x10000);
}

void run_umsboot(void** firmware, void** app, int* size)
{
    fastboot_umsboot(firmware, app, size);
    if (!*firmware)
    {
        memcpy(framebuf, bg, 320 * 240 * 3);
        message(91, "Loading UMSboot failed!", "Returning to main menu.");
    }
}
