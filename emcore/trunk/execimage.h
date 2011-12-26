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


#ifndef __EXECIMAGE_H__
#define __EXECIMAGE_H__


#ifdef _TOOL
#include <stdint.h>
#else
#include "global.h"
#include "thread.h"
#endif


#define EMCOREAPP_HEADER_VERSION 1
struct emcoreapp_header
{
	char signature[8];
	uint32_t version;
    uint32_t textstart;
    uint32_t textsize;
    uint32_t bsssize;
    uint32_t stacksize;
	uint32_t entrypoint;
	uint32_t relocstart;
	uint32_t reloccount;
	uint32_t flags;
    uint32_t creationtime;
};

#define EMCOREAPP_FLAG_COMPRESSED 0x00000001
#define EMCOREAPP_FLAG_LIBRARY    0x00000002


#ifndef _TOOL
struct scheduler_thread* execimage(void* image, bool copy, int argc, const char* const* argv);
#endif


#endif
