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
#include "confirmchooser.h"
#include "main.h"
#include "util.h"
#include "settings.h"


static struct chooser_renderer_list_itemdata confirmchooser_rparams_yes =
{
    .size = LIBUI_POINT(260, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(260, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0xa00000ff,
    .icon_pos = LIBUI_POINT_NULL,
    .icon = LIBUI_SURFACE_NULL,
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE_NULL,
    .icon_selected_opacity = 0,
    .text = "Yes",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xff0000ff,
    .text_color_selected = 0xff3f7fff,
    .render = NULL
};

static struct chooser_renderer_list_itemdata confirmchooser_rparams_no =
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
    .text = "No",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff,
    .render = NULL
};

static struct chooser_renderer_list_params confirmchooser_rparams =
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
    .viewport = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(30, 70)),
                              LIBUI_POINT(260, 140)),
    .blit_dest = LIBUI_POINT(0, 0),
    .blit_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 320), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(320, 240)),
    .preblit = update_display,
    .postblit = NULL
};

static struct chooser_action_handler_wheel_params confirmchooser_aparams =
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

static struct chooser_info confirmchooser =
{
    .version = CHOOSER_INFO_VERSION,
    .actionhandler = NULL,
    .actionhandlerparams = &confirmchooser_aparams,
    .renderer = NULL,
    .rendererparams = &confirmchooser_rparams,
    .userparams = NULL,
    .tickinterval = 10000000,
    .itemcount = 14,
    .defaultitem = 0,
    .items =
    {
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = (void*)1,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_yes
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &confirmchooser_rparams_no
        }
    }
};

bool run_confirmchooser(const char* message)
{
    memcpy(framebuf2, bg, 320 * 240 * 3);
    ui->blendcolor(260, 20, 0xa0000000, framebuf2, 30, 50, 320, framebuf2, 30, 50, 320);
    rendertext(framebuf2, 31, 51, 320, 0xff3333ff, 0, message);
    const struct chooser_item* result = ui->chooser_run(&confirmchooser);
    if (!result || !result->user) return false;
    return true;
}

void confirmchooser_init()
{
    confirmchooser.actionhandler = ui->chooser_action_handler_wheel;
    confirmchooser.renderer = ui->chooser_renderer_list;
    confirmchooser_rparams.copy_dest.buf.addr = framebuf;
    confirmchooser_rparams.copy_src.loc.buf.addr = framebuf2;
    confirmchooser_rparams.viewport.loc.buf.addr = framebuf;
    confirmchooser_rparams.blit_src.loc.buf.addr = framebuf;
    confirmchooser_rparams_yes.render = ui->chooser_renderer_list_show_arrow_right;
    confirmchooser_rparams_no.render = ui->chooser_renderer_list_show_arrow_left;
}

void confirmchooser_apply_settings()
{
    if (settings.snow) confirmchooser.tickinterval = 50000;
    else confirmchooser.tickinterval = 10000000;
}
