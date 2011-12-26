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


void run_powerdown()
{
    shutdown(true);
    power_off();
}

void fastboot_rockbox()
{
    boot->load_from_file(&bootinfo.firmware, &bootinfo.size, true, "/.rockbox/rockbox.ipod", 0);
    if (bootinfo.firmware) bootinfo.valid = true;
}

void run_rockbox_fallback()
{
    boot->load_from_flash(&bootinfo.firmware, &bootinfo.size, true, "rockbox ", 0x100000);
    if (bootinfo.firmware) bootinfo.valid = true;
    else
    {
        memcpy(framebuf, bg, 320 * 240 * 3);
        message(91, "Loading Rockbox failed!", "Returning to main menu.");
    }
}

void run_rockbox()
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
    fastboot_rockbox();
    if (!bootinfo.valid)
    {
        message(76, "Loading rockbox.ipod failed!", "  Trying fallback image...  ");
        run_rockbox_fallback();
    }
}

void fastboot_umsboot()
{
    boot->load_from_flash(&bootinfo.firmware, &bootinfo.size, false, "umsboot ", 0x10000);
    if (bootinfo.firmware) bootinfo.valid = true;
}

void run_umsboot()
{
    fastboot_umsboot();
    if (!bootinfo.valid)
    {
        memcpy(framebuf, bg, 320 * 240 * 3);
        message(91, "Loading UMSboot failed!", "Returning to main menu.");
    }
}
