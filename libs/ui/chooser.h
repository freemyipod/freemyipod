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


#ifndef __CHOOSER_H__
#define __CHOOSER_H__

#include "emcorelib.h"
#include "libui.h"


enum chooser_result
{
    CHOOSER_RESULT_OK = 0,
    CHOOSER_RESULT_REDRAW,
    CHOOSER_RESULT_FINISHED,
    CHOOSER_RESULT_CANCEL
};


struct chooser_item
{
    const void* user;
    const void* actionparams;
    const void* renderparams;
};

struct chooser_data;

#define CHOOSER_ACTION_HANDLER_VERSION 1

struct chooser_action_handler
{
    int version;
    int (*init)(struct chooser_data* data);
    enum chooser_result (*handleevent)(struct chooser_data* data,
                                       enum button_event event, int which, int value);
    enum chooser_result (*handletick)(struct chooser_data* data);
    int (*stepsperitem)(struct chooser_data* data);
    void (*destroy)(struct chooser_data* data);
};

#define CHOOSER_RENDERER_VERSION 1

struct chooser_renderer
{
    int version;
    int (*init)(struct chooser_data* data);
    enum chooser_result (*render)(struct chooser_data* data);
    const struct chooser_item* (*itematpixel)(struct chooser_data* data, int x, int y);
    void (*destroy)(struct chooser_data* data);
};

#define CHOOSER_INFO_VERSION 1

struct chooser_info
{
    int version;
    const struct chooser_action_handler* actionhandler;
    const void* actionhandlerparams;
    const struct chooser_renderer* renderer;
    const void* rendererparams;
    const void* userparams;
    long tickinterval;
    int itemcount;
    int defaultitem;
    const struct chooser_item items[];
};

struct chooser_data
{
    const struct chooser_info* info;
    struct mutex statemutex;
    struct wakeup eventwakeup;
    bool redrawneeded;
    bool finished;
    bool canceled;
    const struct chooser_item* selected;
    int position;
    void* actionhandlerdata;
    void* rendererdata;
    void* userdata;
};


const struct chooser_item* chooser_run(const struct chooser_info* info);


#endif
