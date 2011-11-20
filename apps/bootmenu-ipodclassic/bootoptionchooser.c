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
#include "bootoptionchooser.h"
#include "main.h"
#include "util.h"


static struct chooser_renderer_list_itemdata bootoptionchooser_rparams_cancel =
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
    .text = "Cancel",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata bootoptionchooser_rparams_option_0 =
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
    .text = NULL,
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata bootoptionchooser_rparams_option_1 =
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
    .text = NULL,
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata bootoptionchooser_rparams_option_2 =
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
    .text = NULL,
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_itemdata bootoptionchooser_rparams_option_3 =
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
    .text = NULL,
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

static struct chooser_renderer_list_params bootoptionchooser_rparams =
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

static struct chooser_action_handler_wheel_params bootoptionchooser_aparams =
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

static struct chooser_info bootoptionchooser =
{
    .version = CHOOSER_INFO_VERSION,
    .actionhandler = NULL,
    .actionhandlerparams = &bootoptionchooser_aparams,
    .renderer = NULL,
    .rendererparams = &bootoptionchooser_rparams,
    .userparams = NULL,
    .tickinterval = 10000000,
    .itemcount = 5,
    .defaultitem = 0,
    .items =
    {
        {
            .user = (void*)-1,
            .actionparams = NULL,
            .renderparams = &bootoptionchooser_rparams_cancel
        },
        {
            .user = (void*)0,
            .actionparams = NULL,
            .renderparams = &bootoptionchooser_rparams_option_0
        },
        {
            .user = (void*)1,
            .actionparams = NULL,
            .renderparams = &bootoptionchooser_rparams_option_1
        },
        {
            .user = (void*)2,
            .actionparams = NULL,
            .renderparams = &bootoptionchooser_rparams_option_2
        },
        {
            .user = (void*)3,
            .actionparams = NULL,
            .renderparams = &bootoptionchooser_rparams_option_3
        }
    }
};

int run_bootoptionchooser(int selected_index, const char* n0, const char* n1,
                          const char* n2, const char* n3)
{
    bootoptionchooser.defaultitem = selected_index + 1;
    bootoptionchooser_rparams_option_0.text = n0;
    bootoptionchooser_rparams_option_1.text = n1;
    bootoptionchooser_rparams_option_2.text = n2;
    bootoptionchooser_rparams_option_3.text = n3;
    const struct chooser_item* result = ui->chooser_run(&bootoptionchooser);
    if (!result) return -1;
    return (int)result->user;
}

void bootoptionchooser_init()
{
    bootoptionchooser.actionhandler = ui->chooser_action_handler_wheel;
    bootoptionchooser.renderer = ui->chooser_renderer_list;
    bootoptionchooser_rparams.copy_dest.buf.addr = framebuf;
    bootoptionchooser_rparams.copy_src.loc.buf.addr = bg;
    bootoptionchooser_rparams.viewport.loc.buf.addr = framebuf;
    bootoptionchooser_rparams.blit_src.loc.buf.addr = framebuf;
}
