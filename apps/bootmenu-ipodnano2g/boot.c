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

void fastboot_crapple(void** firmware, void** app, int* size)
{
    boot->load_from_file(firmware, size, false, "/.boot/appleos.ucl", 0x800000);
    if (!*firmware) boot->load_from_file(firmware, size, false, "/.boot/appleos.bin", 0);
}

void run_crapple(void** firmware, void** app, int* size)
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
    fastboot_crapple(firmware, app, size);
    if (!*firmware) message(7, "Loading appleos.bin failed!", "  Returning to main menu.  ");
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
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(19, "Loading Rockbox failed!", "Returning to main menu.");
    }
}

void run_rockbox(void** firmware, void** app, int* size)
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
    fastboot_rockbox(firmware, app, size);
    if (!*firmware)
    {
        message(4, "Loading rockbox.ipod failed!", "  Trying fallback image...  ");
        run_rockbox_fallback(firmware, app, size);
    }
}

void fastboot_diskmode(void** firmware, void** app, int* size)
{
    boot->load_from_flash(firmware, size, false, "diskmode", 0x100000);
}

void run_diskmode(void** firmware, void** app, int* size)
{
    fastboot_diskmode(firmware, app, size);
    if (!*firmware)
    {
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(13, "Loading disk mode failed!", " Returning to main menu. ");
    }
}

void run_diagmode(void** firmware, void** app, int* size)
{
    boot->load_from_flash(firmware, size, false, "diagmode", 0x100000);
    if (!*firmware)
    {
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(16, "Loading diag mode failed!", " Returning to main menu. ");
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
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(19, "Loading UMSboot failed!", "Returning to main menu.");
    }
}

void run_uninstaller(void** firmware, void** app, int* size)
{
    boot->load_from_flash(app, size, false, "uninst  ", 0);
    if (!*app)
    {
        memcpy(framebuf, bg, 176 * 132 * 3);
        message(7, "Loading uninstaller failed!", "  Returning to main menu.  ");
    }
}
