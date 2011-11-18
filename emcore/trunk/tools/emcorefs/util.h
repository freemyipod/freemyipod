//
//
//    Copyright 2011 user890104
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


#ifndef __MISC_H__
#define __MISC_H__

#include "global.h"

#define MIN(a,b) ((a)>(b)?(b):(a))


struct alignsizes
{
    uint32_t head;
    uint32_t body;
    uint32_t tail;
};

void dump_packet(const void* data, size_t length);
void alignsplit(struct alignsizes* sizeptr, uint32_t addr, uint32_t size, uint32_t blksize, uint32_t align);
time_t fat_time_to_unix_ts(short wrttime, short wrtdate);
int32_t unix_ts_to_fat_time(time_t datetime);
void print_error(int code);

#endif /* __MISC_H__ */
