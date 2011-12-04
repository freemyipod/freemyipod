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


#ifndef __EXPORT_LIBUI_H__
#define __EXPORT_LIBUI_H__

#include "emcorelib.h"
#include "../libui.h"
#include "../blend.h"
#include "../chooser.h"
#include "../chooser_action_handler_wheel.h"
#include "../chooser_renderer_list.h"
#include "../chooser_renderer_iconflow.h"
#include "../settingchooser.h"


/* emCORE library identifier */
#define LIBUI_IDENTIFIER 0x49554365

/* increase this every time the api struct changes */
#define LIBUI_API_VERSION 3

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define LIBUI_MIN_API_VERSION 3

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase LIBUI_API_VERSION. If you make changes to the
         existing APIs, also update LIBUI_MIN_API_VERSION to current version */

struct libui_api
{
    typeof(fill)* fill;
    typeof(blit)* blit;
    typeof(blendcolor)* blendcolor;
    typeof(mattecolor)* mattecolor;
    typeof(blend)* blend;
    typeof(blend)* blenda;
    typeof(chooser_run)* chooser_run;
    typeof(chooser_action_handler_wheel)* chooser_action_handler_wheel;
    typeof(chooser_renderer_list)* chooser_renderer_list;
    typeof(chooser_renderer_iconflow)* chooser_renderer_iconflow;
    typeof(chooser_renderer_list_render_attached_text)* chooser_renderer_list_render_attached_text;
    typeof(chooser_renderer_list_show_arrow_right)* chooser_renderer_list_show_arrow_right;
    typeof(chooser_renderer_list_show_arrow_left)* chooser_renderer_list_show_arrow_left;
    typeof(settingchooser_run)* settingchooser_run;
};

#endif
