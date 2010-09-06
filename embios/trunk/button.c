//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include "global.h"
#include "button.h"
#include "thread.h"


#ifndef BUTTON_MAX_HOOKS
#define BUTTON_MAX_HOOKS 16
#endif


extern struct scheduler_thread* current_thread;
static struct button_hook_entry button_hooks[BUTTON_MAX_HOOKS] IBSS_ATTR;
static struct mutex button_mutex;


void button_init()
{
    memset(button_hooks, 0, sizeof(button_hooks));
    mutex_init(&button_mutex);
}

int button_register_handler(void (*handler)(enum button_event, int which, int value))
{
    int i;
    mutex_lock(&button_mutex, TIMEOUT_BLOCK);
    for (i = 0; i < BUTTON_MAX_HOOKS; i++)
        if (button_hooks[i].owner == NULL)
        {
            button_hooks[i].owner = current_thread;
            button_hooks[i].handler = handler;
            mutex_unlock(&button_mutex);
            return 0;
        }
    mutex_unlock(&button_mutex);
    return -1;
}

int button_unregister_handler(void (*handler)(enum button_event, int which, int value))
{
    int i;
    mutex_lock(&button_mutex, TIMEOUT_BLOCK);
    for (i = 0; i < BUTTON_MAX_HOOKS; i++)
        if (button_hooks[i].handler == handler)
        {
            button_hooks[i].owner = NULL;
            button_hooks[i].handler = NULL;
            mutex_unlock(&button_mutex);
            return 0;
        }
    mutex_unlock(&button_mutex);
    return -1;
}

void button_send_event(enum button_event eventtype, int which, int value)
{
    DEBUGF("Sending button event: %d, %02X, %02X", eventtype, which, value);
    int i;
    for (i = 0; i < BUTTON_MAX_HOOKS; i++)
        if (button_hooks[i].owner != NULL)
            button_hooks[i].handler(eventtype, which, value);
}

void button_unregister_all_of_thread(struct scheduler_thread* process)
{
    int i;
    mutex_lock(&button_mutex, TIMEOUT_BLOCK);
    for (i = 0; i < BUTTON_MAX_HOOKS; i++)
        if (button_hooks[i].owner == process)
        {
            button_hooks[i].owner = NULL;
            button_hooks[i].handler = NULL;
        }
    mutex_unlock(&button_mutex);
}
