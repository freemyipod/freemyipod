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


#ifndef __BUTTON_H__
#define __BUTTON_H__


#include "global.h"
#include "thread.h"


enum button_event
{
    BUTTON_PRESS,
    BUTTON_RELEASE,
    WHEEL_TOUCH,
    WHEEL_UNTOUCH,
    WHEEL_POSITION,
    WHEEL_MOVED,
    WHEEL_MOVED_ACCEL,
    WHEEL_FORWARD,
    WHEEL_BACKWARD
};

struct button_hook_entry
{
    struct button_hook_entry* next;
    struct scheduler_thread* owner;
    void (*handler)(void*, enum button_event, int, int);
    void* user;
};


void button_init() INITCODE_ATTR;
struct button_hook_entry* button_register_handler(void (*handler)(void*, enum button_event,
                                                                  int, int),
                                                  void* user);
int button_unregister_handler(struct button_hook_entry* hook);
void button_send_event(enum button_event eventtype, int which, int value) ICODE_ATTR;
void button_unregister_all_of_thread(struct scheduler_thread* process);


#endif
