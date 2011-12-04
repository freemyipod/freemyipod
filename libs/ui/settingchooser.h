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


#ifndef __SETTINGCHOOSER_H__
#define __SETTINGCHOOSER_H__

#include "emcorelib.h"
#include "libui.h"
#include "chooser_renderer_list.h"


struct settingchooser_item_config_integer
{
    int min;
    int max;
    int step;
    int (*tostring)(char* buf, int buflen, void* setting, int value);
};

struct settingchooser_select_option
{
    const char* preview;
    const char* text;
    struct libui_surface icon;
    struct libui_surface icon_selected;
};
#define SETTINGCHOOSER_SELECT_OPTION(a, b, c, d) \
{                      \
    .preview = a,      \
    .text = b,         \
    .icon = c,         \
    .icon_selected = d \
}
#define SETTINGCHOOSER_SELECT_OPTION_NULL \
    SETTINGCHOOSER_SELECT_OPTION(NULL, NULL, LIBUI_SURFACE_NULL, LIBUI_SURFACE_NULL)

struct settingchooser_select_options
{
    int optioncount;
    struct settingchooser_select_option options[];
};

struct settingchooser_item_config_select
{
    const struct settingchooser_select_options* options;
};

union settingchooser_item_config
{
    struct settingchooser_item_config_integer integer;
    struct settingchooser_item_config_select select;
};

enum settingchooser_type
{
    SETTINGCHOOSER_TYPE_NULL = 0,
    SETTINGCHOOSER_TYPE_INTEGER,
    SETTINGCHOOSER_TYPE_SELECT
};

struct settingchooser_item
{
    const char* text;
    struct libui_surface icon;
    struct libui_surface icon_selected;
    enum settingchooser_type type;
    void* setting;
    void (*validator)(void* setting);
    union settingchooser_item_config config;
};

struct settingchooser_itemparams
{
    struct libui_point size;
    struct libui_box fill_box;
    uint32_t fill_color;
    uint32_t fill_color_selected;
    uint32_t fill_color_active;
    struct libui_point icon_pos;
    int icon_opacity;
    int icon_selected_opacity;
    int icon_active_opacity;
    struct libui_point text_pos;
    uint32_t text_color;
    uint32_t text_color_selected;
    uint32_t text_color_active;
};

#define SETTINGCHOOSER_INFO_VERSION 1
struct settingchooser_info
{
    int version;
    struct chooser_renderer_list_params rendererparams;
    struct settingchooser_itemparams itemparams;
    const char* returntext;
    long tickinterval;
    int itemcount;
    const struct settingchooser_item items[];
};

struct settingchooser_data
{
    const struct settingchooser_info* info;
    bool changed;
    bool editing;
    int collect;
};


bool settingchooser_run(const struct settingchooser_info* info);


#endif
