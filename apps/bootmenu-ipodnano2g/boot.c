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

void fastboot_crapple()
{
    boot->load_from_file(&bootinfo.firmware, &bootinfo.size, false, "/.boot/appleos.ucl", 0x800000);
    if (!&bootinfo.firmware)
        boot->load_from_file(&bootinfo.firmware, &bootinfo.size, false, "/.boot/appleos.bin", 0);
    if (bootinfo.firmware) bootinfo.valid = true;
}

void run_crapple()
{
    int i;
    for (i = 23; i <= 115; i += 23)
    {
        if (i < 115)
            ui->blend(176, 132, 50, framebuf, 0, 0, 176,
                      framebuf, 0, 0, 176, bg, 0, 0, 176);
        else memcpy(framebuf, bg, 176 * 132 * 3);
        memcpy(framebuf2, framebuf, 176 * 132 * 3);
        ui->blenda(111, i, 255, framebuf2, 32, 0, 176,
                   framebuf2, 32, 0, 176, crapple, 0, 115 - i, 111);
        displaylcd(0, 0, 176, 132, framebuf2, 0, 0, 176);
    }
    fastboot_crapple();
    if (!bootinfo.valid) message(7, "Loading appleos.bin failed!", "  Returning to main menu.  ");
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
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(19, "Loading Rockbox failed!", "Returning to main menu.");
    }
}

void run_rockbox()
{
    int i;
    for (i = 2; i <= 52; i += 10)
    {
        if (i < 52)
            ui->blend(176, 132, 50, framebuf, 0, 0, 176,
                      framebuf, 0, 0, 176, bg, 0, 0, 176);
        else memcpy(framebuf, bg, 176 * 132 * 3);
        ui->blit(154, MIN(47, i), 3, framebuf, 11, MAX(0, i - 47), 176,
                 rbxlogo, 0, MAX(0, 47 - i), 154);
        displaylcd(0, 0, 176, 132, framebuf, 0, 0, 176);
    }
    fastboot_rockbox();
    if (!bootinfo.valid)
    {
        message(4, "Loading rockbox.ipod failed!", "  Trying fallback image...  ");
        run_rockbox_fallback();
    }
}

void fastboot_diskmode()
{
    boot->load_from_flash(&bootinfo.firmware, &bootinfo.size, false, "diskmode", 0x100000);
    if (bootinfo.firmware) bootinfo.valid = true;
}

void run_diskmode()
{
    fastboot_diskmode();
    if (!bootinfo.valid)
    {
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(13, "Loading disk mode failed!", " Returning to main menu. ");
    }
}

void run_diagmode()
{
    boot->load_from_flash(&bootinfo.firmware, &bootinfo.size, false, "diagmode", 0x100000);
    if (bootinfo.firmware) bootinfo.valid = true;
    else
    {
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(16, "Loading diag mode failed!", " Returning to main menu. ");
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
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(19, "Loading UMSboot failed!", "Returning to main menu.");
    }
}

void run_uninstaller()
{
    boot->load_from_flash(&bootinfo.app, &bootinfo.size, false, "uninst  ", 0);
    if (bootinfo.app) bootinfo.valid = true;
    else
    {
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(7, "Loading uninstaller failed!", "  Returning to main menu.  ");
    }
}
