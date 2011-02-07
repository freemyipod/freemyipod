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


#include "global.h"
#include "malloc.h"
#include "libc/tlsf/tlsf.h"


extern char _poolstart;   // These aren't ints at all, but gcc complains about void types being
extern char _poolend;     // used here, and we only need the address, so just make it happy...

struct mutex malloc_mutex;
tlsf_pool global_mallocpool;


void* malloc(size_t size)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    void* ptr = tlsf_malloc(global_mallocpool, size + 4);
    size = tlsf_block_size(ptr);
    *((struct scheduler_thread**)(ptr + size - 4)) = current_thread;
    mutex_unlock(&malloc_mutex);
    return ptr;
}

void* memalign(size_t align, size_t size)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    void* ptr = tlsf_memalign(global_mallocpool, align, size + 4);
    size = tlsf_block_size(ptr);
    *((struct scheduler_thread**)(ptr + size - 4)) = current_thread;
    mutex_unlock(&malloc_mutex);
    return ptr;
}

void* realloc(void* ptr, size_t size)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    size_t oldsize = tlsf_block_size(ptr);
    struct scheduler_thread* owner = *((struct scheduler_thread**)(ptr + size - 4));
    ptr = tlsf_realloc(global_mallocpool, ptr, size + 4);
    size = tlsf_block_size(ptr);
    *((struct scheduler_thread**)(ptr + size - 4)) = owner;
    mutex_unlock(&malloc_mutex);
    return ptr;
}

void reownalloc(void* ptr, struct scheduler_thread* owner)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    size_t size = tlsf_block_size(ptr);
    *((struct scheduler_thread**)(ptr + size - 4)) = owner;
    mutex_unlock(&malloc_mutex);
}

void free(void* ptr)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    tlsf_free(global_mallocpool, ptr);
    mutex_unlock(&malloc_mutex);
}

void free_if_thread(void* ptr, size_t size, int used, void* owner)
{
    if (*((void**)(ptr + size - 4)) == owner)
        tlsf_free(global_mallocpool, ptr);
}

void free_all_of_thread(struct scheduler_thread* owner)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    tlsf_walk_heap(global_mallocpool, free_if_thread, owner);
    mutex_unlock(&malloc_mutex);
}

void malloc_walk(void (*walker), void* user)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    tlsf_walk_heap(global_mallocpool, walker, user);
    mutex_unlock(&malloc_mutex);
}

void malloc_init()
{
    mutex_init(&malloc_mutex);
    global_mallocpool = tlsf_create(&_poolstart, &_poolend - &_poolstart);
}
