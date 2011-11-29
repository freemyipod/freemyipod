//
//
//    Copyright 2010 TheSeven
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
#include "malloc.h"
#include "libc/tlsf/tlsf.h"


extern char _poolstart;   // These aren't chars at all, but gcc complains about void types being
extern char _poolend;     // used here, and we only need the address, so just make it happy...

struct mutex malloc_mutex;
tlsf_pool global_mallocpool;


void* malloc(size_t size)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    void* ptr = tlsf_malloc(global_mallocpool, size + 4);
    if (ptr)
    {
        size = tlsf_block_size(ptr);
        *((struct scheduler_thread**)(ptr + size - 4)) = current_thread;
    }
    DEBUGF("malloc(%08X) => %08X (thread: %08X)", size, ptr, current_thread);
    mutex_unlock(&malloc_mutex);
    return ptr;
}

void* memalign(size_t align, size_t size)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    void* ptr = tlsf_memalign(global_mallocpool, align, size + 4);
    if (ptr)
    {
        size = tlsf_block_size(ptr);
        *((struct scheduler_thread**)(ptr + size - 4)) = current_thread;
    }
    DEBUGF("memalign(%X, %08X) => %08X (thread: %08X)", align, size, ptr, current_thread);
    mutex_unlock(&malloc_mutex);
    return ptr;
}

void* realign(void* ptr, size_t align, size_t size)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    size_t oldsize = tlsf_block_size(ptr);
    struct scheduler_thread* owner = *((struct scheduler_thread**)(ptr + oldsize - 4));
    void* ptr_new = tlsf_realign(global_mallocpool, ptr, align, size + 4);
    if (ptr_new)
    {
        size = tlsf_block_size(ptr_new);
        *((struct scheduler_thread**)(ptr_new + size - 4)) = owner;
    }
    DEBUGF("realign(%08X, %X, %08X) => %08X (old size: %08X, owner: %08X, thread: %08X)",
           ptr, align, size, ptr_new, owner, current_thread);
    mutex_unlock(&malloc_mutex);
    return ptr_new;
}

void* realloc(void* ptr, size_t size)
{
    return realign(ptr, 4, size);
}

void reownalloc(void* ptr, struct scheduler_thread* owner)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    size_t size = tlsf_block_size(ptr);
    DEBUGF("reownalloc(%08X, %08X) (size: %08X, old owner: %08X, thread: %08X)",
           ptr, owner, size, *((struct scheduler_thread**)(ptr + size - 4)), current_thread);
    *((struct scheduler_thread**)(ptr + size - 4)) = owner;
    mutex_unlock(&malloc_mutex);
}

void free(void* ptr)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    size_t size = tlsf_block_size(ptr);
    DEBUGF("free(%08X) (size: %08X, owner: %08X, thread: %08X)", ptr, size,
           *((struct scheduler_thread**)(ptr + size - 4)), current_thread);
    tlsf_free(global_mallocpool, ptr);
    mutex_unlock(&malloc_mutex);
}

void free_if_thread(void* ptr, size_t size, int used, void* owner)
{
    if (*((void**)(ptr + size - 4)) == owner) free(ptr);
}

void free_all_of_thread(struct scheduler_thread* owner)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    DEBUGF("free_all_of_thread(%08X) (thread: %08X)", owner, current_thread);
    tlsf_walk_heap(global_mallocpool, free_if_thread, owner);
    mutex_unlock(&malloc_mutex);
}

void malloc_walk(void (*walker), void* user)
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
    tlsf_walk_heap(global_mallocpool, walker, user);
    mutex_unlock(&malloc_mutex);
}

void malloc_lock()
{
    mutex_lock(&malloc_mutex, TIMEOUT_BLOCK);
}

void malloc_unlock()
{
    mutex_unlock(&malloc_mutex);
}

void malloc_init()
{
    mutex_init(&malloc_mutex);
    global_mallocpool = tlsf_create(&_poolstart, &_poolend - &_poolstart);
}
