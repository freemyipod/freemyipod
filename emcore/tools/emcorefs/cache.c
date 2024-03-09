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

#include "global.h"

#include "cache.h"
#include "emcore.h"
#include "util.h"

struct emcore_dir_entry *emcore_dir_entry_cache = NULL;
size_t emcore_dir_cache_length = 0;

void cache_init(void) {
    int32_t datetime = unix_ts_to_fat_time(time(NULL));
#ifdef DEBUG
    fprintf(stderr, "Init cache...\n");
#endif
    emcore_dir_entry_cache = calloc(sizeof(*emcore_dir_entry_cache), 1);

    emcore_dir_entry_cache->name = strdup("/");
    emcore_dir_entry_cache->attributes = 0x10;
    emcore_dir_entry_cache->size = 0x1000;
    emcore_dir_entry_cache->wrtdate = datetime >> 0x10;
    emcore_dir_entry_cache->wrttime = datetime & 0xffff;

    emcore_dir_cache_length = 1;
#ifdef DEBUG
    fprintf(stderr, "Cache init done!\n");
#endif
}

struct emcore_dir_entry *cache_get(const char *name) {
    size_t i;

    for (i = 0; i < emcore_dir_cache_length; ++i) {
#ifdef DEBUG2
        fprintf(stderr, "cache_get: strcmp([%s], [%s]) == %d\n", name, emcore_dir_entry_cache[i].name, strcmp(name, emcore_dir_entry_cache[i].name));
#endif
        if (strcmp(name, emcore_dir_entry_cache[i].name) == 0) {
            return &emcore_dir_entry_cache[i];
        }
    }

    return NULL;
}

void cache_insert(const char *dir_name, const struct emcore_dir_entry *entry) {
#ifdef DEBUG2
    fprintf(stderr, "CACHE INSERT: dir=[%s], entry=[%s]\n", dir_name, entry->name);
#endif
    void *new_ptr;
    struct emcore_dir_entry *cache_entry;
    char *new_name;
    size_t new_name_len = 1;
    
    if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
        return;
    }
    
    new_name_len += strlen(dir_name) + strlen(entry->name);

    if (strcmp(dir_name, "/") != 0) {
        ++new_name_len;
    }

    new_name = calloc(sizeof(char), new_name_len);
    strcat(new_name, dir_name);

    if (strcmp(dir_name, "/") != 0) {
        strcat(new_name, "/");
    }

    strcat(new_name, entry->name);

    if (cache_get(new_name)) {
        free(new_name);
        return;
    }

    new_ptr = realloc(emcore_dir_entry_cache,
        sizeof(*emcore_dir_entry_cache) * (emcore_dir_cache_length + 1));

    if (!new_ptr) {
        free(new_name);
        return;
    }

    emcore_dir_entry_cache = new_ptr;

    cache_entry = malloc(sizeof(*cache_entry));

    if (!cache_entry) {
        free(new_name);
        return;
    }

    memcpy(cache_entry, entry, sizeof(*entry));

    cache_entry->name = new_name;

    memcpy(emcore_dir_entry_cache + emcore_dir_cache_length, cache_entry, sizeof(*cache_entry));
    free(cache_entry);

#ifdef DEBUG
    fprintf(stderr, "Inserting [%s] to cache\n", emcore_dir_entry_cache[emcore_dir_cache_length].name);
#endif
    ++emcore_dir_cache_length;
}

void cache_remove(const char *name) {
    size_t i;
    void *new_ptr;
    
    for (i = 0; i < emcore_dir_cache_length; ++i) {
#ifdef DEBUG2
        fprintf(stderr, "cache_remove: strcmp([%s], [%s]) == %d\n", name, emcore_dir_entry_cache[i].name, strcmp(name, emcore_dir_entry_cache[i].name));
#endif
        if (strcmp(name, emcore_dir_entry_cache[i].name) == 0) {
#ifdef DEBUG2
            fprintf(stderr, "CACHE REMOVE: [%s]\n", name);
#endif
            free(emcore_dir_entry_cache[i].name);
            
            if (i < emcore_dir_cache_length - 1) {
                memcpy(emcore_dir_entry_cache + i, emcore_dir_entry_cache + i + 1, (emcore_dir_cache_length - i - 1) * sizeof(*emcore_dir_entry_cache));
            }
            
            --emcore_dir_cache_length;
            
            new_ptr = realloc(emcore_dir_entry_cache,
                sizeof(*emcore_dir_entry_cache) * (emcore_dir_cache_length));

            if (!new_ptr) {
                return;
            }

            emcore_dir_entry_cache = new_ptr;
        }
    }
}

void cache_destroy(void) {
#ifdef DEBUG
    fprintf(stderr, "Destroying cache...\n");
#endif
    while (emcore_dir_cache_length) {
        free(emcore_dir_entry_cache[--emcore_dir_cache_length].name);
    }

    free(emcore_dir_entry_cache);
#ifdef DEBUG
    fprintf(stderr, "Cache destroyed!\n");
#endif
}

#ifdef DEBUG2
void cache_dump(void) {
    size_t i;

    for (i = 0; i < emcore_dir_cache_length; ++i) {
        fprintf(stderr, "cache_dump: [%s] / attr: 0x%08x / size: %d / startcluster: %d / ts: %lu\n", emcore_dir_entry_cache[i].name, emcore_dir_entry_cache[i].attributes, emcore_dir_entry_cache[i].size, emcore_dir_entry_cache[i].startcluster, fat_time_to_unix_ts(emcore_dir_entry_cache[i].wrttime, emcore_dir_entry_cache[i].wrtdate));
    }
}
#endif
