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


#ifndef __CHOOSER_RENDERER_ICONFLOW_H__
#define __CHOOSER_RENDERER_ICONFLOW_H__

#include "emcorelib.h"
#include "libui.h"
#include "chooser.h"


#define CHOOSER_RENDERER_ICONFLOW_PARAMS_VERSION 3

struct chooser_renderer_iconflow_params
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
    struct libui_point text_pos;
    struct libui_point blit_dest;
    struct libui_surface blit_src;
    uint32_t smoothness;
    uint32_t startposition;
    uint32_t iconsinview;
    bool (*preblit)(struct chooser_data* data);
    void (*postblit)(struct chooser_data* data);
};

struct chooser_renderer_iconflow_itemdata
{
    struct libui_surface icon;
    struct libui_surface icon_selected;
    const char* text;
    uint32_t text_color;
    uint32_t text_bgcolor;
    void (*render)(struct chooser_data* data, const struct chooser_item* item,
                   bool selected, int x, int y, int opacity, int textx, int texty);
};

struct chooser_renderer_iconflow_data
{
    int viewposition;
    long lastupdate;
    int accumulator;
};


extern const struct chooser_renderer chooser_renderer_iconflow;


#endif
