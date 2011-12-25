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


#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__


#include "emcoreapp.h"
#include "libboot.h"
#include "libpng.h"
#include "libui.h"


struct bootinfo_t
{
    bool valid;
    void* firmware;
    int size;
    void* app;
    int argc;
    const char** argv;
};


extern struct bootinfo_t bootinfo;
extern struct libpng_api* png;
extern struct libboot_api* boot;
extern struct libui_api* ui;
extern void* framebuf;
extern void* framebuf2;
extern void* bg;
extern void* icons;
extern void* rbxlogo;


#endif
