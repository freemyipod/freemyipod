//
//
//    Copyright 2013 user890104
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

void dump_packet(const void *data, size_t length);
time_t fat_time_to_unix_ts(uint16_t wrttime, uint16_t wrtdate);
int32_t unix_ts_to_fat_time(time_t datetime);
void print_error(int32_t code);

#endif /* __MISC_H__ */
