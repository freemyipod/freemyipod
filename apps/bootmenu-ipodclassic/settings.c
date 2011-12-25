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
    .magic = "emCOsett",
    .app = "bootmenu",
    .version = SETTINGS_VERSION,
    .timeout_initial = 30000000,
    .timeout_idle = 300000000,
    .timeout_item = 0,
    .default_item = 1,
    .fastboot_item = 0,
    .snow = 5,
    .brightness = 100
};

struct settingdata settings;

void settings_reset()
{
    memcpy(&settings, &settings_default, sizeof(settings));
}

void setting_validate(void* setting)
{
    if (setting == &settings.timeout_initial)
    {
        if (settings.timeout_initial < SETTINGS_TIMEOUT_INITIAL_MIN)
            settings.timeout_initial = SETTINGS_TIMEOUT_INITIAL_MIN;
        if (settings.timeout_initial > SETTINGS_TIMEOUT_INITIAL_MAX)
            settings.timeout_initial = SETTINGS_TIMEOUT_INITIAL_MAX;
    }
    else if (setting == &settings.timeout_idle)
    {
        if (settings.timeout_idle < SETTINGS_TIMEOUT_IDLE_MIN)
            settings.timeout_idle = SETTINGS_TIMEOUT_IDLE_MIN;
        if (settings.timeout_idle > SETTINGS_TIMEOUT_IDLE_MAX)
            settings.timeout_idle = SETTINGS_TIMEOUT_IDLE_MAX;
    }
    else if (setting == &settings.timeout_item)
    {
        if (settings.timeout_item < SETTINGS_TIMEOUT_ITEM_MIN)
            settings.timeout_item = SETTINGS_TIMEOUT_ITEM_MIN;
        if (settings.timeout_item > SETTINGS_TIMEOUT_ITEM_MAX)
            settings.timeout_item = SETTINGS_TIMEOUT_ITEM_MAX;
    }
    else if (setting == &settings.default_item)
    {
        if (settings.default_item < SETTINGS_DEFAULT_ITEM_MIN)
            settings.default_item = SETTINGS_DEFAULT_ITEM_MIN;
        if (settings.default_item > SETTINGS_DEFAULT_ITEM_MAX)
            settings.default_item = SETTINGS_DEFAULT_ITEM_MAX;
    }
    else if (setting == &settings.fastboot_item)
    {
        if (settings.fastboot_item < SETTINGS_FASTBOOT_ITEM_MIN)
            settings.fastboot_item = SETTINGS_FASTBOOT_ITEM_MIN;
        if (settings.fastboot_item > SETTINGS_FASTBOOT_ITEM_MAX)
            settings.fastboot_item = SETTINGS_FASTBOOT_ITEM_MAX;
    }
    else if (setting == &settings.snow)
    {
        if (settings.snow < SETTINGS_SNOW_MIN)
            settings.snow = SETTINGS_SNOW_MIN;
        if (settings.snow > SETTINGS_SNOW_MAX)
            settings.snow = SETTINGS_SNOW_MAX;
        settingchooser_apply_settings();
    }
    else if (setting == &settings.brightness)
    {
        if (settings.brightness < SETTINGS_BRIGHTNESS_MIN)
            settings.brightness = SETTINGS_BRIGHTNESS_MIN;
        if (settings.brightness > SETTINGS_BRIGHTNESS_MAX)
            settings.brightness = SETTINGS_BRIGHTNESS_MAX;
        backlight_set_brightness(settings.brightness);
    }
}

void settings_validate_all()
{
    if (memcmp(settings.magic, settings_default.magic, sizeof(settings.magic))
     || memcmp(settings.app, settings_default.app, sizeof(settings.app))
     || settings.version != SETTINGS_VERSION)
        settings_reset();
    setting_validate(&settings.timeout_initial);
    setting_validate(&settings.timeout_idle);
    setting_validate(&settings.timeout_item);
    setting_validate(&settings.default_item);
    setting_validate(&settings.fastboot_item);
    setting_validate(&settings.snow);
    setting_validate(&settings.brightness);
}

void settings_apply()
{
    mainchooser_apply_settings();
    toolchooser_apply_settings();
    settingchooser_apply_settings();
    confirmchooser_apply_settings();
    backlight_set_brightness(settings.brightness);
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
