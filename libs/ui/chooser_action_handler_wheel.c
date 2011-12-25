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
#include "chooser.h"
#include "chooser_action_handler_wheel.h"


static int chooser_action_handler_wheel_init(struct chooser_data* data)
{
    const struct chooser_action_handler_wheel_params* params;
    params = (const struct chooser_action_handler_wheel_params*)(data->info->actionhandlerparams);
    if (params->version != CHOOSER_ACTION_HANDLER_WHEEL_PARAMS_VERSION) return -1;
    data->actionhandlerdata = malloc(sizeof(struct chooser_action_handler_wheel_data));
    if (!data->actionhandlerdata) return -2;
    struct chooser_action_handler_wheel_data* adata;
    adata = (struct chooser_action_handler_wheel_data*)(data->actionhandlerdata);
    adata->timeout_remaining = params->timeout_initial;
    adata->lasttick = USEC_TIMER;
    return 0;
}

static enum chooser_result chooser_action_handler_wheel_handleevent(struct chooser_data* data,
                                                                    enum button_event event,
                                                                    int which, int value)
{
    const struct chooser_action_handler_wheel_params* params;
    params = (const struct chooser_action_handler_wheel_params*)(data->info->actionhandlerparams);
    struct chooser_action_handler_wheel_data* adata;
    adata = (struct chooser_action_handler_wheel_data*)(data->actionhandlerdata);
    if (params->eventfilter && params->eventfilter(data, event, which, value))
        return CHOOSER_RESULT_OK;
    adata->timeout_remaining = params->timeout_idle;
    int spi = params->stepsperitem;
    switch (event)
    {
        case BUTTON_PRESS:
            if (which < params->buttoncount)
                switch (params->buttonmap[which])
                {
                    case CHOOSER_ACTION_HANDLER_WHEEL_ACTION_PREV:
                        mutex_lock(&data->statemutex, TIMEOUT_BLOCK);
                        data->position = MIN(data->info->itemcount * spi,
                                             MAX(spi, data->position & ~(spi - 1))) - spi / 2;
                        data->selected = &data->info->items[data->position / spi];
                        mutex_unlock(&data->statemutex);
                        return CHOOSER_RESULT_REDRAW;
                    case CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NEXT:
                        mutex_lock(&data->statemutex, TIMEOUT_BLOCK);
                        data->position = MIN(data->info->itemcount * spi,
                                             MAX(spi, (data->position & ~(spi - 1)) + 2 * spi))
                                                    - spi / 2;
                        data->selected = &data->info->items[data->position / spi];
                        mutex_unlock(&data->statemutex);
                        return CHOOSER_RESULT_REDRAW;
                    case CHOOSER_ACTION_HANDLER_WHEEL_ACTION_SELECT:
                        return CHOOSER_RESULT_FINISHED;
                    case CHOOSER_ACTION_HANDLER_WHEEL_ACTION_CANCEL:
                        return CHOOSER_RESULT_CANCEL;
                }
        case WHEEL_MOVED_ACCEL:
            mutex_lock(&data->statemutex, TIMEOUT_BLOCK);
            data->position = MIN(data->info->itemcount * spi - 1, MAX(0, data->position + value));
            data->selected = &data->info->items[data->position / spi];
            mutex_unlock(&data->statemutex);
            return CHOOSER_RESULT_REDRAW;
    }
    return CHOOSER_RESULT_OK;
}

static enum chooser_result chooser_action_handler_wheel_handletick(struct chooser_data* data)
{
    enum chooser_result rc = CHOOSER_RESULT_OK;
    const struct chooser_action_handler_wheel_params* params;
    params = (const struct chooser_action_handler_wheel_params*)(data->info->actionhandlerparams);
    struct chooser_action_handler_wheel_data* adata;
    adata = (struct chooser_action_handler_wheel_data*)(data->actionhandlerdata);
    if (params->tick_force_redraw) rc = CHOOSER_RESULT_REDRAW;
    if (adata->timeout_remaining == TIMEOUT_BLOCK) return rc;
    long time = USEC_TIMER;
    adata->timeout_remaining -= time - adata->lasttick;
    adata->lasttick = time;
    if (adata->timeout_remaining < 0)
    {
        if (params->timeout_item == CHOOSER_ACTION_HANDLER_WHEEL_TIMEOUT_ITEM_NULL)
            return CHOOSER_RESULT_CANCEL;
        else if (params->timeout_item != CHOOSER_ACTION_HANDLER_WHEEL_TIMEOUT_ITEM_KEEP)
            data->selected = &data->info->items[params->timeout_item];
        return CHOOSER_RESULT_FINISHED;
    }
    return rc;
}

static int chooser_action_handler_wheel_stepsperitem(struct chooser_data* data)
{
    const struct chooser_action_handler_wheel_params* params;
    params = (const struct chooser_action_handler_wheel_params*)(data->info->actionhandlerparams);
    return params->stepsperitem;
}

static void chooser_action_handler_wheel_destroy(struct chooser_data* data)
{
    free(data->actionhandlerdata);
}


const struct chooser_action_handler chooser_action_handler_wheel =
{
    .version = CHOOSER_ACTION_HANDLER_VERSION,
    .init = chooser_action_handler_wheel_init,
    .handleevent = chooser_action_handler_wheel_handleevent,
    .handletick = chooser_action_handler_wheel_handletick,
    .stepsperitem = chooser_action_handler_wheel_stepsperitem,
    .destroy = chooser_action_handler_wheel_destroy
};
