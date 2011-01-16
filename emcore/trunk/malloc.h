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


#ifndef __MALLOC_H__
#define __MALLOC_H__


#include "global.h"
#include "thread.h"


void* malloc(size_t size) ICODE_ATTR;
void* memalign(size_t align, size_t size) ICODE_ATTR;
void* realloc(void* ptr, size_t size) ICODE_ATTR;
void reownalloc(void* ptr, struct scheduler_thread* owner);
void free(void* ptr) ICODE_ATTR;
void free_all_of_thread(struct scheduler_thread* owner);
void malloc_init() INITCODE_ATTR;


#endif
