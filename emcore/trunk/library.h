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


#ifndef __LIBRARY_H__
#define __LIBRARY_H__


#ifdef _TOOL
#include <stdint.h>
#else
#include "global.h"
#include "thread.h"
#endif


#define EMCORELIB_HEADER_VERSION 1
struct emcorelib_header
{
    uint32_t headerversion;
    uint32_t identifier;
	uint32_t version;
	uint32_t minversion;
    int (*setupfunc)();
    int (*initfunc)();
    int (*shutdownfunc)();
    void* api;
};

struct library_handle
{
    struct library_handle* next;
    struct emcorelib_header* lib;
    void* alloc;
    void* users[16];
    void** moreusers;
    size_t moreusers_size;
};

enum library_sourcetype
{
    LIBSOURCE_NONE = 0,
    LIBSOURCE_RAM_ALLOCED = 1,
    LIBSOURCE_RAM_NEEDCOPY = 2,
    LIBSOURCE_BOOTFLASH = 3,
    LIBSOURCE_FILESYSTEM = 4
};


#ifndef _TOOL
struct library_handle* library_register(void* image, struct emcorelib_header* header);
int library_unload(struct emcorelib_header* lib);
struct emcorelib_header* get_library(uint32_t identifier, uint32_t version,
                                     enum library_sourcetype sourcetype, void* source);
struct emcorelib_header* get_library_ext(uint32_t identifier, uint32_t version,
                                         enum library_sourcetype sourcetype, void* source,
                                         struct scheduler_thread* owner);
int release_library(struct emcorelib_header* lib);
int release_library_ext(struct emcorelib_header* lib, struct scheduler_thread* owner);
int library_release_all_of_thread(struct scheduler_thread* thread);
#endif


#endif
