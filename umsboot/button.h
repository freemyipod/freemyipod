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
    WHEEL_FORWARD,
    WHEEL_BACKWARD
};

struct button_hook_entry
{
    struct scheduler_thread* owner;
    void (*handler)(enum button_event, int which, int value);
};


void button_init() INITCODE_ATTR;
int button_register_handler(void (*handler)(enum button_event, int which, int value));
int button_unregister_handler(void (*handler)(enum button_event, int which, int value));
void button_send_event(enum button_event eventtype, int which, int value) ICODE_ATTR;
void button_unregister_all_of_thread(struct scheduler_thread* process);


#endif
