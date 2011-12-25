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
#include "libui.h"
#include "settingchooser.h"
#include "main.h"
#include "util.h"
#include "settings.h"


int settingchooser_time_to_str(char* buf, int buflen, void* setting, int value)
{
    if (value < SETTINGS_TIMEOUT_CUTOFF) return snprintf(buf, buflen, "%s", "Never");
    return snprintf(buf, buflen, "%dsec", value / 1000000);
}

static struct settingchooser_select_options settings_timeout_item_options =
{
    .optioncount = 4,
    .options =
    {
        SETTINGCHOOSER_SELECT_OPTION("Power off", "Power off", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("Rockbox", "Rockbox", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("UMSboot", "UMSboot", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("Console", "emCORE console", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL)
    }
};

static struct settingchooser_select_options settings_default_item_options =
{
    .optioncount = 4,
    .options =
    {
        SETTINGCHOOSER_SELECT_OPTION("Power off", "Power off", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("Rockbox", "Rockbox", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("Console", "emCORE console", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("Tools", "Tools", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL)
    }
};

static struct settingchooser_select_options settings_fastboot_item_options =
{
    .optioncount = 4,
    .options =
    {
        SETTINGCHOOSER_SELECT_OPTION("Disabled", "Disabled", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("Rockbox", "Rockbox", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("UMSboot", "UMSboot", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL),
        SETTINGCHOOSER_SELECT_OPTION("Console", "emCORE console", LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL)
    }
};

static struct settingchooser_info settingchooser =
{
    .version = SETTINGCHOOSER_INFO_VERSION,
    .rendererparams = 
    {
        .version = CHOOSER_RENDERER_LIST_PARAMS_VERSION,
        .copy_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
        .copy_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
                                  LIBUI_POINT(320, 240)),
        .bg_dest = LIBUI_LOCATION_NULL,
        .bg_src = LIBUI_SURFACE_NULL,
        .bg_opacity = 0,
        .fill_dest = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
        .fill_color = 0,
        .viewport = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(30, 50)),
                                  LIBUI_POINT(260, 160)),
        .blit_dest = LIBUI_POINT(0, 0),
        .blit_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
                                  LIBUI_POINT(320, 240)),
        .preblit = update_display,
        .postblit = NULL
    },
    .itemparams =
    {
        .size = LIBUI_POINT(260, 10),
        .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
        .fill_color = 0xa0000000,
        .fill_color_selected = 0x60000000,
        .fill_color_active = 0xa0003f3f,
        .icon_pos = LIBUI_POINT_NULL,
        .icon_opacity = 0,
        .icon_selected_opacity = 0,
        .icon_active_opacity = 0,
        .text_pos = LIBUI_POINT(1, 1),
        .text_color = 0xffffffff,
        .text_color_selected = 0xff7fffff,
        .text_color_active = 0xffff7f7f
    },
    .returntext = "Return to tools menu",
    .tickinterval = 10000000,
    .itemcount = 7,
    .items =
    {
        {
            .text = "Initial timeout",
            .icon = LIBUI_SURFACE_NULL,
            .icon_selected = LIBUI_SURFACE_NULL,
            .type = SETTINGCHOOSER_TYPE_INTEGER,
            .setting = &settings.timeout_initial,
            .validator = setting_validate,
            .config.integer =
            {
                .min = SETTINGS_TIMEOUT_INITIAL_MIN,
                .max = SETTINGS_TIMEOUT_INITIAL_MAX,
                .step = SETTINGS_TIMEOUT_INITIAL_STEP,
                .tostring = settingchooser_time_to_str
            }
        },
        {
            .text = "Idle timeout",
            .icon = LIBUI_SURFACE_NULL,
            .icon_selected = LIBUI_SURFACE_NULL,
            .type = SETTINGCHOOSER_TYPE_INTEGER,
            .setting = &settings.timeout_idle,
            .validator = setting_validate,
            .config.integer =
            {
                .min = SETTINGS_TIMEOUT_IDLE_MIN,
                .max = SETTINGS_TIMEOUT_IDLE_MAX,
                .step = SETTINGS_TIMEOUT_IDLE_STEP,
                .tostring = settingchooser_time_to_str
            }
        },
        {
            .text = "Timeout action",
            .icon = LIBUI_SURFACE_NULL,
            .icon_selected = LIBUI_SURFACE_NULL,
            .type = SETTINGCHOOSER_TYPE_SELECT,
            .setting = &settings.timeout_item,
            .validator = setting_validate,
            .config.select =
            {
                .options = &settings_timeout_item_options
            }
        },
        {
            .text = "Default action",
            .icon = LIBUI_SURFACE_NULL,
            .icon_selected = LIBUI_SURFACE_NULL,
            .type = SETTINGCHOOSER_TYPE_SELECT,
            .setting = &settings.default_item,
            .validator = setting_validate,
            .config.select =
            {
                .options = &settings_default_item_options
            }
        },
        {
            .text = "Fastboot action",
            .icon = LIBUI_SURFACE_NULL,
            .icon_selected = LIBUI_SURFACE_NULL,
            .type = SETTINGCHOOSER_TYPE_SELECT,
            .setting = &settings.fastboot_item,
            .validator = setting_validate,
            .config.select =
            {
                .options = &settings_fastboot_item_options
            }
        },
        {
            .text = "Snow",
            .icon = LIBUI_SURFACE_NULL,
            .icon_selected = LIBUI_SURFACE_NULL,
            .type = SETTINGCHOOSER_TYPE_INTEGER,
            .setting = &settings.snow,
            .validator = setting_validate,
            .config.integer =
            {
                .min = SETTINGS_SNOW_MIN,
                .max = SETTINGS_SNOW_MAX,
                .step = SETTINGS_SNOW_STEP,
                .tostring = NULL
            }
        },
        {
            .text = "Backlight brightness",
            .icon = LIBUI_SURFACE_NULL,
            .icon_selected = LIBUI_SURFACE_NULL,
            .type = SETTINGCHOOSER_TYPE_INTEGER,
            .setting = &settings.brightness,
            .validator = setting_validate,
            .config.integer =
            {
                .min = SETTINGS_BRIGHTNESS_MIN,
                .max = SETTINGS_BRIGHTNESS_MAX,
                .step = SETTINGS_BRIGHTNESS_STEP,
                .tostring = NULL
            }
        }
    }
};

void run_settingchooser()
{
    bool changes = ui->settingchooser_run(&settingchooser);
    if (changes)
    {
        rendertext(framebuf, 106, 140, 320, 0xff33ffff, 0xa0000000, "Saving settings...");
        displaylcd(0, 0, 320, 240, framebuf, 0, 0, 320);
        settings_validate_all();
        settings_save();
        settings_apply();
    }
}

void settingchooser_init()
{
    settingchooser.rendererparams.copy_dest.buf.addr = framebuf;
    settingchooser.rendererparams.copy_src.loc.buf.addr = bg;
    settingchooser.rendererparams.viewport.loc.buf.addr = framebuf;
    settingchooser.rendererparams.blit_src.loc.buf.addr = framebuf;
}

void settingchooser_apply_settings()
{
    if (settings.snow) settingchooser.tickinterval = 50000;
    else settingchooser.tickinterval = 10000000;
}
