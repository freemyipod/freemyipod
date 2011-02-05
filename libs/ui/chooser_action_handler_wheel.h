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


#ifndef __CHOOSER_ACTION_HANDLER_WHEEL_H__
#define __CHOOSER_ACTION_HANDLER_WHEEL_H__

#include "emcorelib.h"
#include "libui.h"
#include "chooser.h"


enum chooser_action_handler_wheel_action
{
    CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NONE = 0,
    CHOOSER_ACTION_HANDLER_WHEEL_ACTION_PREV,
    CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NEXT,
    CHOOSER_ACTION_HANDLER_WHEEL_ACTION_SELECT,
    CHOOSER_ACTION_HANDLER_WHEEL_ACTION_CANCEL
};

#define CHOOSER_ACTION_HANDLER_WHEEL_TIMEOUT_ITEM_NULL -1
#define CHOOSER_ACTION_HANDLER_WHEEL_TIMEOUT_ITEM_KEEP -2

#define CHOOSER_ACTION_HANDLER_WHEEL_PARAMS_VERSION 1

struct chooser_action_handler_wheel_params
{
    int version;
    int stepsperitem;
    bool (*eventfilter)(struct chooser_data* data, enum button_event event, int which, int value);
    long timeout_initial;
    long timeout_idle;
    int timeout_item;
    bool tick_force_redraw;
    int buttoncount;
    enum chooser_action_handler_wheel_action buttonmap[];
};

struct chooser_action_handler_wheel_data
{
    long timeout_remaining;
    long lasttick;
};


extern const struct chooser_action_handler chooser_action_handler_wheel;


#endif
