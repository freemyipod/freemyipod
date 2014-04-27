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


#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__


#include "emcoreapp.h"


#define SETTINGS_VERSION 1
struct settingdata
{
    char magic[8];
    char app[8];
    uint32_t version;
    long timeout_initial;
    long timeout_idle;
    int timeout_item;
    int default_item;
    int fastboot_item;
    int snow;
    int brightness;
};

#define SETTINGS_TIMEOUT_INITIAL_MIN 0
#define SETTINGS_TIMEOUT_INITIAL_MAX 2000000000
#define SETTINGS_TIMEOUT_INITIAL_STEP 1000000
#define SETTINGS_TIMEOUT_IDLE_MIN 0
#define SETTINGS_TIMEOUT_IDLE_MAX 2000000000
#define SETTINGS_TIMEOUT_IDLE_STEP 1000000
#define SETTINGS_TIMEOUT_ITEM_MIN 0
#define SETTINGS_TIMEOUT_ITEM_MAX 4
#define SETTINGS_DEFAULT_ITEM_MIN 0
#define SETTINGS_DEFAULT_ITEM_MAX 4
#define SETTINGS_FASTBOOT_ITEM_MIN 0
#define SETTINGS_FASTBOOT_ITEM_MAX 4
#define SETTINGS_SNOW_MIN 0
#define SETTINGS_SNOW_MAX 10
#define SETTINGS_SNOW_STEP 1
#define SETTINGS_BRIGHTNESS_MIN 20
#define SETTINGS_BRIGHTNESS_MAX 255
#define SETTINGS_BRIGHTNESS_STEP 5

#define SETTINGS_TIMEOUT_CUTOFF 3000000


extern struct settingdata settings_default;
extern struct settingdata settings;


extern void settings_init();
extern void settings_reset();
extern void setting_validate(void* setting);
extern void settings_validate_all();
extern void settings_apply();
extern void settings_save();


#endif
