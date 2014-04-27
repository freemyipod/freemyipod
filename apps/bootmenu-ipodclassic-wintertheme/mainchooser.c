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
#include "mainchooser.h"
#include "toolchooser.h"
#include "settings.h"
#include "util.h"
#include "main.h"
#include "boot.h"


static bool mainchooser_preblit(struct chooser_data* data)
{
    char buf[4];
    struct chooser_action_handler_wheel_data* adata;
    adata = (struct chooser_action_handler_wheel_data*)(data->actionhandlerdata);
    if (adata->timeout_remaining != TIMEOUT_BLOCK)
    {
        snprintf(buf, sizeof(buf), "%3d", adata->timeout_remaining / 1000000);
        rendertext(framebuf, 299, 229, 320, 0xcf7f0000, 0, buf);
    }
    update_display(data);
    return false;
}

static struct chooser_renderer_iconflow_itemdata mainchooser_rparams_powerdown =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(80, 80)),
    .text = "Power off",
    .text_color = 0xffffcccc,
    .text_bgcolor = 0x7f000000,
    .render = NULL
};

static struct chooser_renderer_iconflow_itemdata mainchooser_rparams_rockbox =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 80)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 80)),
                                   LIBUI_POINT(80, 80)),
    .text = "Rockbox",
    .text_color = 0xffffcccc,
    .text_bgcolor = 0x7f000000,
    .render = NULL
};

struct chooser_renderer_iconflow_itemdata mainchooser_rparams_diskmode =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 160)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 160)),
                                   LIBUI_POINT(80, 80)),
    .text = "Disk mode",
    .text_color = 0xffffcccc,
    .text_bgcolor = 0x7f000000,
    .render = NULL
};

static struct chooser_renderer_iconflow_itemdata mainchooser_rparams_console =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 240)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 240)),
                                   LIBUI_POINT(80, 80)),
    .text = "emCORE console",
    .text_color = 0xffffcccc,
    .text_bgcolor = 0x7f000000,
    .render = NULL
};

static struct chooser_renderer_iconflow_itemdata mainchooser_rparams_toolchooser =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 320)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 320)),
                                   LIBUI_POINT(80, 80)),
    .text = "Tools",
    .text_color = 0xffffcccc,
    .text_bgcolor = 0x7f000000,
    .render = NULL
};

static struct chooser_renderer_iconflow_params mainchooser_rparams =
{
    .version = CHOOSER_RENDERER_ICONFLOW_PARAMS_VERSION,
    .copy_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
    .copy_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(320, 240)),
    .bg_dest = LIBUI_LOCATION_NULL,
    .bg_src = LIBUI_SURFACE_NULL,
    .bg_opacity = 0,
    .fill_dest = LIBUI_SURFACE_NULL,
    .fill_color = 0,
    .viewport = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 30)),
                              LIBUI_POINT(320, 130)),
    .text_pos = LIBUI_POINT(160, 215),
    .blit_dest = LIBUI_POINT(0, 0),
    .blit_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(320, 240)),
    .smoothness = 500000,
    .startposition = -3,
    .iconsinview = 4,
    .preblit = mainchooser_preblit,
    .postblit = NULL
};

static struct chooser_action_handler_wheel_params mainchooser_aparams =
{
    .version = CHOOSER_ACTION_HANDLER_WHEEL_PARAMS_VERSION,
    .stepsperitem = 512,
    .eventfilter = NULL,
    .timeout_initial = 0,
    .timeout_idle = 0,
    .timeout_item = CHOOSER_ACTION_HANDLER_WHEEL_TIMEOUT_ITEM_NULL,
    .tick_force_redraw = true,
    .buttoncount = 3,
    .buttonmap =
    {
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_SELECT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NEXT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_PREV
    }
};

static struct chooser_info mainchooser =
{
    .version = CHOOSER_INFO_VERSION,
    .actionhandler = NULL,
    .actionhandlerparams = &mainchooser_aparams,
    .renderer = NULL,
    .rendererparams = &mainchooser_rparams,
    .userparams = NULL,
    .tickinterval = 990000,
    .itemcount = 5,
    .defaultitem = 0,
    .items =
    {
        {
            .user = run_powerdown,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_powerdown
        },
        {
            .user = run_rockbox,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_rockbox
        },
        {
            .user = run_diskmode,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_diskmode
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_console
        },
        {
            .user = run_toolchooser,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_toolchooser
        }
    }
};

void run_mainchooser()
{
    while (!bootinfo.valid)
    {
        const struct chooser_item* result = ui->chooser_run(&mainchooser);
        if (!result)
            switch(settings.timeout_item)
            {
                case 0:
                    run_powerdown();
                    break;
                
                case 1:
                    run_rockbox();
                    break;
                
                case 2:
                    run_diskmode();
                    break;
                
                case 3:
                    run_umsboot();
                    break;
                
                case 4:
                    return;
            }
        if (!result->user) return;
        ((void(*)())(result->user))();
    }
}

void mainchooser_init()
{
    mainchooser.actionhandler = ui->chooser_action_handler_wheel;
    mainchooser.renderer = ui->chooser_renderer_iconflow;
    mainchooser_rparams.copy_dest.buf.addr = framebuf;
    mainchooser_rparams.copy_src.loc.buf.addr = bg;
    mainchooser_rparams.viewport.loc.buf.addr = framebuf;
    mainchooser_rparams.blit_src.loc.buf.addr = framebuf;
    mainchooser_rparams_powerdown.icon.loc.buf.addr = icons;
    mainchooser_rparams_powerdown.icon_selected.loc.buf.addr = icons;
    mainchooser_rparams_rockbox.icon.loc.buf.addr = icons;
    mainchooser_rparams_rockbox.icon_selected.loc.buf.addr = icons;
    mainchooser_rparams_console.icon.loc.buf.addr = icons;
    mainchooser_rparams_console.icon_selected.loc.buf.addr = icons;
    mainchooser_rparams_diskmode.icon.loc.buf.addr = icons;
    mainchooser_rparams_diskmode.icon_selected.loc.buf.addr = icons;
    mainchooser_rparams_toolchooser.icon.loc.buf.addr = icons;
    mainchooser_rparams_toolchooser.icon_selected.loc.buf.addr = icons;
}

void mainchooser_apply_settings()
{
    mainchooser.defaultitem = settings.default_item;
    if (settings.snow) mainchooser.tickinterval = 50000;
    else mainchooser.tickinterval = 990000;
    if (settings.timeout_initial < SETTINGS_TIMEOUT_CUTOFF)
        mainchooser_aparams.timeout_initial = TIMEOUT_BLOCK;
    else mainchooser_aparams.timeout_initial = settings.timeout_initial + 500000;
    if (settings.timeout_idle < SETTINGS_TIMEOUT_CUTOFF)
        mainchooser_aparams.timeout_idle = TIMEOUT_BLOCK;
    else mainchooser_aparams.timeout_idle = settings.timeout_idle + 500000;
}
