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


static struct chooser_renderer_list_itemdata settingchooser_rparams_toolchooser =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Return to tools menu",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata settingchooser_rparams_timeout_initial =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Initial timeout",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata settingchooser_rparams_timeout_idle =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Idle timeout",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata settingchooser_rparams_timeout_item =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Timeout boot option",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata settingchooser_rparams_default_item =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Default boot option",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata settingchooser_rparams_fastboot_item =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Fastboot boot option",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_params settingchooser_rparams =
{
    .version = CHOOSER_RENDERER_LIST_PARAMS_VERSION,
    .copy_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
    .copy_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(320, 240)),
    .bg_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
    .bg_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                            LIBUI_POINT(0, 0)),
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
};

static struct chooser_action_handler_wheel_params settingchooser_aparams =
{
    .version = CHOOSER_ACTION_HANDLER_WHEEL_PARAMS_VERSION,
    .stepsperitem = 128,
    .eventfilter = NULL,
    .timeout_initial = TIMEOUT_BLOCK,
    .timeout_idle = TIMEOUT_BLOCK,
    .timeout_item = 0,
    .tick_force_redraw = false,
    .buttoncount = 5,
    .buttonmap =
    {
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_SELECT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NONE,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_CANCEL,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NEXT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_PREV
    }
};

static struct chooser_info settingchooser =
{
    .version = CHOOSER_INFO_VERSION,
    .actionhandler = NULL,
    .actionhandlerparams = &settingchooser_aparams,
    .renderer = NULL,
    .rendererparams = &settingchooser_rparams,
    .userparams = NULL,
    .tickinterval = 10000000,
    .itemcount = 6,
    .defaultitem = 0,
    .items =
    {
        {
            .user = (void*)0,
            .actionparams = NULL,
            .renderparams = &settingchooser_rparams_toolchooser
        },
        {
            .user = (void*)1,
            .actionparams = NULL,
            .renderparams = &settingchooser_rparams_timeout_initial
        },
        {
            .user = (void*)2,
            .actionparams = NULL,
            .renderparams = &settingchooser_rparams_timeout_idle
        },
        {
            .user = (void*)3,
            .actionparams = NULL,
            .renderparams = &settingchooser_rparams_timeout_item
        },
        {
            .user = (void*)4,
            .actionparams = NULL,
            .renderparams = &settingchooser_rparams_default_item
        },
        {
            .user = (void*)5,
            .actionparams = NULL,
            .renderparams = &settingchooser_rparams_fastboot_item
        }
    }
};

void run_settingchooser(void** firmware, void** app, int* size)
{
    while (true)
    {
        const struct chooser_item* result = ui->chooser_run(&settingchooser);
        if (!result || !result->user)
        {
            rendertext(framebuf, 106, 140, 320, 0xff33ffff, 0xa0000000, "Saving settings...");
            displaylcd(0, 0, 320, 240, framebuf, 0, 0, 320);
            settings_validate_all();
            settings_save();
            settings_apply();
            return;
        }
        int new;
        switch ((int)result->user)
        {
            case 3:
                new = run_bootoptionchooser(settings.timeout_item, "Power off", "Rockbox", "UMSboot", "emCORE console");
                if (new != -1) settings.timeout_item = new;
                break;
                
            case 4:
                new = run_bootoptionchooser(settings.default_item, "Power off", "Rockbox", "emCORE console", "Tools");
                if (new != -1) settings.default_item = new;
                break;
                
            case 5:
                new = run_bootoptionchooser(settings.fastboot_item, "Disabled", "Rockbox", "UMSboot", "emCORE console");
                if (new != -1) settings.fastboot_item = new;
                break;
        }
    }
}

void settingchooser_init()
{
    settingchooser.actionhandler = ui->chooser_action_handler_wheel;
    settingchooser.renderer = ui->chooser_renderer_list;
    settingchooser_rparams.copy_dest.buf.addr = framebuf;
    settingchooser_rparams.copy_src.loc.buf.addr = bg;
    settingchooser_rparams.viewport.loc.buf.addr = framebuf;
    settingchooser_rparams.blit_src.loc.buf.addr = framebuf;
}
