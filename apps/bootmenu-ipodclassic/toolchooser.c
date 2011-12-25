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
#include "toolchooser.h"
#include "boot.h"
#include "tools.h"
#include "main.h"
#include "util.h"
#include "settings.h"
#include "settingchooser.h"


static struct chooser_renderer_list_itemdata toolchooser_rparams_mainchooser =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60000000,
    .icon_pos = LIBUI_POINT_NULL,
    .icon = LIBUI_SURFACE_NULL,
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE_NULL,
    .icon_selected_opacity = 0,
    .text = "Return to main menu",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff,
    .render = NULL
};

static struct chooser_renderer_list_itemdata toolchooser_rparams_umsboot =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60000000,
    .icon_pos = LIBUI_POINT_NULL,
    .icon = LIBUI_SURFACE_NULL,
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE_NULL,
    .icon_selected_opacity = 0,
    .text = "Run UMSboot",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff,
    .render = NULL
};

static struct chooser_renderer_list_itemdata toolchooser_rparams_rockbox_fallback =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60000000,
    .icon_pos = LIBUI_POINT_NULL,
    .icon = LIBUI_SURFACE_NULL,
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE_NULL,
    .icon_selected_opacity = 0,
    .text = "Run Rockbox fallback image",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff,
    .render = NULL
};

static struct chooser_renderer_list_itemdata toolchooser_rparams_clearcfg =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60000000,
    .icon_pos = LIBUI_POINT_NULL,
    .icon = LIBUI_SURFACE_NULL,
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE_NULL,
    .icon_selected_opacity = 0,
    .text = "Clear Rockbox configuration",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff,
    .render = NULL
};

static struct chooser_renderer_list_itemdata toolchooser_rparams_cleardb =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60000000,
    .icon_pos = LIBUI_POINT_NULL,
    .icon = LIBUI_SURFACE_NULL,
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE_NULL,
    .icon_selected_opacity = 0,
    .text = "Clear Rockbox database",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff,
    .render = NULL
};

static struct chooser_renderer_list_itemdata toolchooser_rparams_reformat =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60000000,
    .icon_pos = LIBUI_POINT_NULL,
    .icon = LIBUI_SURFACE_NULL,
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE_NULL,
    .icon_selected_opacity = 0,
    .text = "Reformat data partition",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff,
    .render = NULL
};

static struct chooser_renderer_list_itemdata toolchooser_rparams_settingchooser =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60000000,
    .icon_pos = LIBUI_POINT_NULL,
    .icon = LIBUI_SURFACE_NULL,
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE_NULL,
    .icon_selected_opacity = 0,
    .text = "Settings",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff,
    .render = NULL
};

static struct chooser_renderer_list_params toolchooser_rparams =
{
    .version = CHOOSER_RENDERER_LIST_PARAMS_VERSION,
    .copy_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
    .copy_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(320, 240)),
    .bg_dest = LIBUI_LOCATION_NULL,
    .bg_src = LIBUI_SURFACE_NULL,
    .bg_opacity = 0,
    .fill_dest = LIBUI_SURFACE_NULL,
    .fill_color = 0,
    .viewport = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(30, 50)),
                              LIBUI_POINT(260, 160)),
    .blit_dest = LIBUI_POINT(0, 0),
    .blit_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(320, 240)),
    .preblit = update_display,
    .postblit = NULL
};

static struct chooser_action_handler_wheel_params toolchooser_aparams =
{
    .version = CHOOSER_ACTION_HANDLER_WHEEL_PARAMS_VERSION,
    .stepsperitem = 128,
    .eventfilter = NULL,
    .timeout_initial = TIMEOUT_BLOCK,
    .timeout_idle = TIMEOUT_BLOCK,
    .timeout_item = 0,
    .tick_force_redraw = true,
    .buttoncount = 5,
    .buttonmap =
    {
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_SELECT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_SELECT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_CANCEL,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NEXT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_PREV
    }
};

static struct chooser_info toolchooser =
{
    .version = CHOOSER_INFO_VERSION,
    .actionhandler = NULL,
    .actionhandlerparams = &toolchooser_aparams,
    .renderer = NULL,
    .rendererparams = &toolchooser_rparams,
    .userparams = NULL,
    .tickinterval = 10000000,
    .itemcount = 7,
    .defaultitem = 0,
    .items =
    {
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_mainchooser
        },
        {
            .user = run_umsboot,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_umsboot
        },
        {
            .user = run_rockbox_fallback,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_rockbox_fallback
        },
        {
            .user = run_clearcfg,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_clearcfg
        },
        {
            .user = run_cleardb,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_cleardb
        },
        {
            .user = run_reformat,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_reformat
        },
        {
            .user = run_settingchooser,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_settingchooser
        }
    }
};

void run_toolchooser()
{
    while (!bootinfo.valid)
    {
        const struct chooser_item* result = ui->chooser_run(&toolchooser);
        if (!result || !result->user) return;
        ((void(*)())(result->user))();
    }
}

void toolchooser_init()
{
    toolchooser.actionhandler = ui->chooser_action_handler_wheel;
    toolchooser.renderer = ui->chooser_renderer_list;
    toolchooser_rparams.copy_dest.buf.addr = framebuf;
    toolchooser_rparams.copy_src.loc.buf.addr = bg;
    toolchooser_rparams.viewport.loc.buf.addr = framebuf;
    toolchooser_rparams.blit_src.loc.buf.addr = framebuf;
    toolchooser_rparams_mainchooser.render = ui->chooser_renderer_list_show_arrow_left;
    toolchooser_rparams_settingchooser.render = ui->chooser_renderer_list_show_arrow_right;
}

void toolchooser_apply_settings()
{
    if (settings.snow) toolchooser.tickinterval = 50000;
    else toolchooser.tickinterval = 10000000;
}
