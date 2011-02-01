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


#ifndef __CHOOSER_RENDERER_LIST_H__
#define __CHOOSER_RENDERER_LIST_H__

#include "emcorelib.h"
#include "libui.h"
#include "chooser.h"


#define CHOOSER_RENDERER_LIST_PARAMS_VERSION 1

struct chooser_renderer_list_params
{
    int version;
    struct libui_location copy_dest;
    struct libui_surface copy_src;
    struct libui_location bg_dest;
    struct libui_surface bg_src;
    int bg_opacity;
    struct libui_surface fill_dest;
    uint32_t fill_color;
    struct libui_surface viewport;
    struct libui_point blit_dest;
    struct libui_surface blit_src;
    bool (*preblit)(struct chooser_data* data);
    void (*postblit)(struct chooser_data* data);
};

struct chooser_renderer_list_itemdata
{
    struct libui_point size;
    struct libui_box fill_box;
    uint32_t fill_color;
    uint32_t fill_color_selected;
    struct libui_point icon_pos;
    struct libui_surface icon;
    int icon_opacity;
    struct libui_surface icon_selected;
    int icon_selected_opacity;
    char* text;
    struct libui_point text_pos;
    uint32_t text_color;
    uint32_t text_color_selected;
};

struct chooser_renderer_list_data
{
    const struct chooser_item* top_item;
    const struct chooser_item* bottom_item;
};


extern const struct chooser_renderer chooser_renderer_list;


#endif
