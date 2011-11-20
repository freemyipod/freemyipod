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
#include "settings.h"
#include "mainchooser.h"


struct settingdata settings_default =
{
    .version = SETTINGS_VERSION,
    .timeout_initial = 30000000,
    .timeout_idle = 300000000,
    .timeout_item = 0,
    .default_item = 1,
    .fastboot_item = 0
};

struct settingdata settings;

void settings_reset()
{
    memcpy(&settings, &settings_default, sizeof(settings));
}

void settings_validate_all()
{
    if (settings.version != SETTINGS_VERSION) settings_reset();
    if (settings.timeout_initial < 0) settings.timeout_initial = 0;
    if (settings.timeout_initial > 2000000000) settings.timeout_initial = 2000000000;
    if (settings.timeout_idle < 0) settings.timeout_idle = 0;
    if (settings.timeout_idle > 2000000000) settings.timeout_idle = 2000000000;
    if (settings.timeout_item < 0) settings.timeout_item = 0;
    if (settings.timeout_item > 3) settings.timeout_item = 3;
    if (settings.default_item < 0) settings.default_item = 0;
    if (settings.default_item > 3) settings.default_item = 3;
    if (settings.fastboot_item < 0) settings.fastboot_item = 0;
    if (settings.fastboot_item > 3) settings.fastboot_item = 3;
}

void settings_apply()
{
    mainchooser_apply_settings();
}

void settings_init()
{
    settings_reset();
    int fd = file_open("/.apps/bootmenu/data/settings.dat", O_RDONLY);
    if (fd > 0)
    {
        int size = filesize(fd);
        if (size > 0)
        {
            if (size > sizeof(settings)) size = sizeof(settings);
            read(fd, &settings, size);
        }
        close(fd);
    }
    settings_validate_all();
    settings_apply();
}

void settings_save()
{
    mkdir("/.apps");
    mkdir("/.apps/bootmenu");
    mkdir("/.apps/bootmenu/data");
    int fd = file_creat("/.apps/bootmenu/data/settings.dat");
    if (fd > 0)
    {
        write(fd, &settings, sizeof(settings));
        close(fd);
    }
}
