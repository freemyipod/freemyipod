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

#ifndef __CACHE_H__
#define __CACHE_H__

#include "global.h"

void cache_init(void);
struct emcore_dir_entry *cache_get(const char *name);
void cache_insert(const char *dir_name, const struct emcore_dir_entry *entry);
void cache_remove(const char *dir_name);
void cache_destroy(void);

#ifdef DEBUG2
void cache_dump(void);
#endif

#endif /* __CACHE_H__ */
