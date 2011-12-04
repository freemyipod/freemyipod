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


#ifndef __LIBUI_H__
#define __LIBUI_H__


struct libui_buffer
{
    void* addr;
    int stride;
};
#define LIBUI_BUFFER(a, b) \
{                          \
    .addr = a,             \
    .stride = b            \
}
#define LIBUI_BUFFER_NULL LIBUI_BUFFER(NULL, 0)

struct libui_point
{
    int x;
    int y;
};
#define LIBUI_POINT(a, b) \
{                         \
    .x = a,               \
    .y = b                \
}
#define LIBUI_POINT_NULL LIBUI_POINT(0, 0)

struct libui_box
{
    struct libui_point pos;
    struct libui_point size;
};
#define LIBUI_BOX(a, b) \
{                       \
    .pos = a,           \
    .size = b           \
}
#define LIBUI_BOX_NULL LIBUI_BOX(LIBUI_POINT_NULL, LIBUI_POINT_NULL)

struct libui_location
{
    struct libui_buffer buf;
    struct libui_point pos;
};
#define LIBUI_LOCATION(a, b) \
{                            \
    .buf = a,                \
    .pos = b                 \
}
#define LIBUI_LOCATION_NULL LIBUI_LOCATION(LIBUI_BUFFER_NULL, LIBUI_POINT_NULL)

struct libui_surface
{
    struct libui_location loc;
    struct libui_point size;
};
#define LIBUI_SURFACE(a, b) \
{                           \
    .loc = a,               \
    .size = b               \
}
#define LIBUI_SURFACE_NULL LIBUI_SURFACE(LIBUI_LOCATION_NULL, LIBUI_POINT_NULL)

#endif
