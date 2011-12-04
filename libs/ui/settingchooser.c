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


#include "emcorelib.h"
#include "libui.h"
#include "settingchooser.h"
#include "chooser.h"
#include "chooser_renderer_list.h"
#include "chooser_action_handler_wheel.h"


static bool settingchooser_event_filter(struct chooser_data* data, enum button_event event,
                                        int which, int value)
{
    struct settingchooser_data* state = (struct settingchooser_data*)data->info->userparams;
    struct settingchooser_info* info = (struct settingchooser_info*)state->info;
    struct settingchooser_item* item = (struct settingchooser_item*)data->selected->user;
    struct chooser_renderer_list_itemdata* rp;
    rp = (struct chooser_renderer_list_itemdata*)data->selected->renderparams;
    bool redraw = false;
    bool handled = false;
    bool setcolors = false;
    switch (event)
    {
        case BUTTON_PRESS:
            switch (which)
            {
                case 0:
                case 1:
                    switch (item->type)
                    {
                        case SETTINGCHOOSER_TYPE_INTEGER:
                            handled = true;
                            setcolors = true;
                            state->editing = !state->editing;
                            if (state->editing) state->collect = 0;
                            break;
                    }
                    break;

                case 2:
                    if (state->editing)
                    {
                        handled = true;
                        setcolors = true;
                        state->editing = false;
                    }
                    break;

                default:
                    handled = state->editing;
                    break;
            }
            break;
            
        case WHEEL_MOVED_ACCEL:
            if (state->editing)
            {
                switch (item->type)
                {
                    case SETTINGCHOOSER_TYPE_INTEGER:
                        handled = true;
                        state->collect += value;
                        int change = (state->collect * item->config.integer.step) / 128;
                        if (!change) break;
                        state->collect -= (change * 128) / item->config.integer.step;
                        int* setting = (int*)item->setting;
                        if (*setting + change < item->config.integer.min)
                            *setting = item->config.integer.min;
                        else if (*setting + change > item->config.integer.max)
                            *setting = item->config.integer.max;
                        else *setting += change;
                        if (item->validator) item->validator(item->setting);
                        state->changed = true;
                        redraw = true;
                        break;
                }
            }
            break;
    }
    if (setcolors)
    {
        if (state->editing)
        {
            rp->fill_color_selected = info->itemparams.fill_color_active;
            rp->text_color_selected = info->itemparams.text_color_active;
            rp->icon_selected_opacity = info->itemparams.icon_active_opacity;
        }
        else
        {
            rp->fill_color_selected = info->itemparams.fill_color_selected;
            rp->text_color_selected = info->itemparams.text_color_selected;
            rp->icon_selected_opacity = info->itemparams.icon_selected_opacity;
        }
        redraw = true;
    }
    if (redraw)
    {
        mutex_lock(&data->statemutex, TIMEOUT_BLOCK);
        data->redrawneeded = true;
        wakeup_signal(&data->eventwakeup);
        mutex_unlock(&data->statemutex);
    }
    return handled;
}

static struct chooser_action_handler_wheel_params settingchooser_aparams =
{
    .version = CHOOSER_ACTION_HANDLER_WHEEL_PARAMS_VERSION,
    .stepsperitem = 128,
    .eventfilter = settingchooser_event_filter,
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
static struct chooser_action_handler_wheel_params settingchooser_aparams2 =
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

static void settingchooser_render_preview(struct chooser_data* data,
                                          const struct chooser_item* item,
                                          bool selected, int x, int y)
{
    struct settingchooser_item* itemdata = (struct settingchooser_item*)item->user;
    int value = *((int*)itemdata->setting);
    const char* str = NULL;
    char buf[16];
    switch (itemdata->type)
    {
        case SETTINGCHOOSER_TYPE_INTEGER:
            if (itemdata->config.integer.tostring)
                itemdata->config.integer.tostring(buf, sizeof(buf), itemdata->setting, value);
            else snprintf(buf, sizeof(buf), "%d", value);
            str = buf;
            break;

        case SETTINGCHOOSER_TYPE_SELECT:
            if (value < itemdata->config.select.options->optioncount)
                str = itemdata->config.select.options->options[value].preview;
            break;
    }
    if (str) chooser_renderer_list_render_attached_text(data, item, selected, x, y, str);
}

static void settingchooser_populate_rp(const struct settingchooser_info* info,
                                       struct chooser_renderer_list_itemdata* rp)
{
    rp->size = info->itemparams.size;
    rp->fill_box = info->itemparams.fill_box;
    rp->fill_color = info->itemparams.fill_color;
    rp->fill_color_selected = info->itemparams.fill_color_selected;
    rp->icon_pos = info->itemparams.icon_pos;
    rp->icon_opacity = info->itemparams.icon_opacity;
    rp->icon_selected_opacity = info->itemparams.icon_selected_opacity;
    rp->text_pos = info->itemparams.text_pos;
    rp->text_color = info->itemparams.text_color;
    rp->text_color_selected = info->itemparams.text_color_selected;
}

bool settingchooser_run(const struct settingchooser_info* info)
{
    int i;
    bool rc = false;
    struct chooser_renderer_list_itemdata* rp;
    if (info->version != SETTINGCHOOSER_INFO_VERSION) return false;
    int allocsize = sizeof(struct settingchooser_data)
                  + sizeof(struct chooser_info)
                  + sizeof(struct chooser_item) * (info->itemcount + 1)
                  + sizeof(struct chooser_renderer_list_itemdata) * (info->itemcount + 1);
    void* mem = malloc(allocsize);
    if (!mem) return false;
    memset(mem, 0, allocsize);
    void* ptr = mem;
    struct settingchooser_data* data = (struct settingchooser_data*)ptr;
    ptr += sizeof(struct settingchooser_data);
    struct chooser_info* chooser = (struct chooser_info*)ptr;
    ptr += sizeof(struct chooser_info);
    ptr += sizeof(struct chooser_item) * (info->itemcount + 1);
    for (i = 0; i < info->itemcount + 1; i++)
    {
        chooser->items[i].renderparams = (struct chooser_renderer_list_itemdata*)ptr;
        ptr += sizeof(struct chooser_renderer_list_itemdata);
    }
    data->info = info;
    chooser->version = CHOOSER_INFO_VERSION;
    chooser->actionhandler = &chooser_action_handler_wheel;
    chooser->actionhandlerparams = &settingchooser_aparams;
    chooser->renderer = &chooser_renderer_list;
    chooser->rendererparams = &info->rendererparams;
    chooser->userparams = data;
    chooser->tickinterval = info->tickinterval;
    chooser->itemcount = info->itemcount + 1;
    rp = (struct chooser_renderer_list_itemdata*)chooser->items[0].renderparams;
    settingchooser_populate_rp(info, rp);
    rp->text = info->returntext;
    rp->render = chooser_renderer_list_show_arrow_left;
    for (i = 0; i < info->itemcount; i++)
    {
        chooser->items[i + 1].user = &info->items[i];
        rp = (struct chooser_renderer_list_itemdata*)chooser->items[i + 1].renderparams;
        settingchooser_populate_rp(info, rp);
        rp->text = info->items[i].text;
        rp->icon = info->items[i].icon;
        rp->icon_selected = info->items[i].icon_selected;
        rp->render = settingchooser_render_preview;
    }
    while (true)
    {
        const struct chooser_item* result = chooser_run(chooser);
        if (!result) break;
        struct settingchooser_item* item = (struct settingchooser_item*)result->user;
        if (!item) break;
        chooser->defaultitem = result - chooser->items;
        const struct settingchooser_select_options* options = item->config.select.options;
        int allocsize2 = sizeof(struct chooser_info)
                       + sizeof(struct chooser_item) * (options->optioncount + 1)
                       + sizeof(struct chooser_renderer_list_itemdata)
                       * (options->optioncount + 1);
        void* mem2 = malloc(allocsize2);
        if (!mem2) continue;
        memset(mem2, 0, allocsize2);
        void* ptr = mem2;
        struct chooser_info* chooser2 = (struct chooser_info*)ptr;
        ptr += sizeof(struct chooser_info);
        ptr += sizeof(struct chooser_item) * (options->optioncount + 1);
        for (i = 0; i < options->optioncount + 1; i++)
        {
            chooser2->items[i].renderparams = (struct chooser_renderer_list_itemdata*)ptr;
            ptr += sizeof(struct chooser_renderer_list_itemdata);
        }
        chooser2->version = CHOOSER_INFO_VERSION;
        chooser2->actionhandler = &chooser_action_handler_wheel;
        chooser2->actionhandlerparams = &settingchooser_aparams;
        chooser2->renderer = &chooser_renderer_list;
        chooser2->rendererparams = &info->rendererparams;
        chooser2->userparams = data;
        chooser2->tickinterval = info->tickinterval;
        chooser2->itemcount = options->optioncount + 1;
        chooser2->defaultitem = *((int*)item->setting) + 1;
        rp = (struct chooser_renderer_list_itemdata*)chooser2->items[0].renderparams;
        settingchooser_populate_rp(info, rp);
        rp->text = "Cancel";
        rp->render = chooser_renderer_list_show_arrow_left;
        for (i = 0; i < options->optioncount; i++)
        {
            chooser2->items[i + 1].user = &options->options[i];
            rp = (struct chooser_renderer_list_itemdata*)chooser2->items[i + 1].renderparams;
            settingchooser_populate_rp(info, rp);
            rp->text = options->options[i].text;
            rp->icon = options->options[i].icon;
            rp->icon_selected = options->options[i].icon_selected;
        }
        result = chooser_run(chooser2);
        int value = result - chooser2->items;
        if (result && value && *((int*)item->setting) != value - 1)
        {
            data->changed = true;
            *((int*)item->setting) = value - 1;
            if (item->validator) item->validator(item->setting);
        }
        free(mem2);
    }
    rc = data->changed;
    free(mem);
    return rc;
}
