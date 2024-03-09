//
//
//    Copyright 2010 TheSeven
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


#include "global.h"
#include "button.h"
#include "thread.h"


static struct button_hook_entry* head_button_hook IBSS_ATTR;
static struct mutex button_mutex;


void button_init()
{
    head_button_hook = NULL;
    mutex_init(&button_mutex);
}

struct button_hook_entry* button_register_handler(void (*handler)(void*, enum button_event,
                                                                  int, int),
                                                  void* user)
{
    struct button_hook_entry* hook;
    hook = (struct button_hook_entry*)malloc(sizeof(struct button_hook_entry));
    if (!hook) return NULL;
    hook->owner = current_thread;
    hook->handler = handler;
    hook->user = user;
    mutex_lock(&button_mutex, TIMEOUT_BLOCK);
    hook->next = head_button_hook;
    head_button_hook = hook;
    mutex_unlock(&button_mutex);
    return hook;
}

int button_unregister_handler(struct button_hook_entry* hook)
{
    struct button_hook_entry* h;
    int result = -1;
    mutex_lock(&button_mutex, TIMEOUT_BLOCK);
    if (head_button_hook == hook)
    {
        head_button_hook = head_button_hook->next;
        free(hook);
        result = 0;
    }
    else
    {
        for (h = head_button_hook; h && h->next; h = h->next);
            if (h->next == hook)
            {
                h->next = h->next->next;
                free(hook);
                result = 0;
            }
    }
    mutex_unlock(&button_mutex);
    return result;
}

void button_send_event(enum button_event eventtype, int which, int value)
{
    DEBUGF("Sending button event: %d, %02X, %02X", eventtype, which, value);
    struct button_hook_entry* h;
    mutex_lock(&button_mutex, TIMEOUT_BLOCK);
    for (h = head_button_hook; h; h = h->next)
        h->handler(h->user, eventtype, which, value);
    mutex_unlock(&button_mutex);
}

void button_unregister_all_of_thread(struct scheduler_thread* process)
{
    struct button_hook_entry* h;
    struct button_hook_entry* prev;
    mutex_lock(&button_mutex, TIMEOUT_BLOCK);
    while (head_button_hook && head_button_hook->owner == process)
    {
        prev = head_button_hook;
        head_button_hook = head_button_hook->next;
        free(prev);
    }
    for (h = head_button_hook; h; h = h->next)
        while (h->next && h->next->owner == process)
        {
            prev = h->next;
            h->next = h->next->next;
            free(prev);
        }
    mutex_unlock(&button_mutex);
}
