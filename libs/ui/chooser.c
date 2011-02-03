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


void chooser_button_handler(void* user, enum button_event event, int which, int value)
{
    struct chooser_data* data = (struct chooser_data*)user;
    switch (data->info->actionhandler->handleevent(data, event, which, value))
    {
    case CHOOSER_RESULT_REDRAW:
        mutex_lock(&data->statemutex, TIMEOUT_BLOCK);
        data->redrawneeded = true;
        wakeup_signal(&data->eventwakeup);
        mutex_unlock(&data->statemutex);
        break;
    case CHOOSER_RESULT_CANCEL:
        mutex_lock(&data->statemutex, TIMEOUT_BLOCK);
        data->canceled = true;
        wakeup_signal(&data->eventwakeup);
        mutex_unlock(&data->statemutex);
        break;
    case CHOOSER_RESULT_FINISHED:
        mutex_lock(&data->statemutex, TIMEOUT_BLOCK);
        data->finished = true;
        wakeup_signal(&data->eventwakeup);
        mutex_unlock(&data->statemutex);
        break;
    }
}

const struct chooser_item* chooser_run(const struct chooser_info* info)
{
    struct chooser_data data;
    const struct chooser_item* rc = NULL;
    if (info->version != CHOOSER_INFO_VERSION) goto done;
    if (!info->actionhandler) goto done;
    if (info->actionhandler->version != CHOOSER_ACTION_HANDLER_VERSION) goto done;
    if (!info->renderer) goto done;
    if (info->renderer->version != CHOOSER_RENDERER_VERSION) goto done;
    if (info->defaultitem >= info->itemcount) data.selected = &info->items[info->itemcount - 1];
    else data.selected = &info->items[info->defaultitem];
    data.info = info;
    data.redrawneeded = false;
    data.canceled = false;
    data.finished = false;
    data.position = 0;
    data.actionhandlerdata = NULL;
    data.rendererdata = NULL;
    mutex_init(&data.statemutex);
    wakeup_init(&data.eventwakeup);
    if (info->actionhandler->init)
        if (info->actionhandler->init(&data) < 0)
            goto done;
    data.selected = &info->items[info->defaultitem];
    int spi = info->actionhandler->stepsperitem(&data);
    data.position = info->defaultitem * spi + spi / 2;
    if (info->renderer->init)
        if (info->renderer->init(&data) < 0)
            goto destroy_actionhandler;
    struct button_hook_entry* hook = button_register_handler(chooser_button_handler, &data);
    if (!hook) goto destroy_renderer;
    long lasttick = USEC_TIMER;
    bool redrawneeded = true;
    while (true)
    {
        long sleeptime = lasttick + info->tickinterval - USEC_TIMER;
        if (sleeptime > 0 && !redrawneeded) wakeup_wait(&data.eventwakeup, sleeptime);
        mutex_lock(&data.statemutex, TIMEOUT_BLOCK);
        if (data.canceled) goto cancel;
        if (data.finished) goto finished;
        redrawneeded |= data.redrawneeded;
        data.redrawneeded = false;
        mutex_unlock(&data.statemutex);
        if ((long)(lasttick + info->tickinterval - USEC_TIMER) < 0)
        {
            if (info->actionhandler->handletick)
                switch (info->actionhandler->handletick(&data))
                {
                case CHOOSER_RESULT_REDRAW:
                    redrawneeded = true;
                    break;
                case CHOOSER_RESULT_CANCEL:
                    goto cancel;
                case CHOOSER_RESULT_FINISHED:
                    goto finished;
                }
            lasttick = USEC_TIMER;
        }
        if (redrawneeded) redrawneeded = info->renderer->render(&data) == CHOOSER_RESULT_REDRAW;
    }
cancel:
    data.selected = NULL;
finished:
    rc = data.selected;
destroy_buttonhook:
    button_unregister_handler(hook);
destroy_renderer:
    if (info->renderer->destroy)
        info->renderer->destroy(&data);
destroy_actionhandler:
    if (info->actionhandler->destroy)
        info->actionhandler->destroy(&data);
done:
    return rc;
}
