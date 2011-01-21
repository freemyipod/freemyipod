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
#include "library.h"
#include "malloc.h"
#include "execimage.h"
#include "thread.h"
#ifdef HAVE_STORAGE
#include "file.h"
#include "dir.h"
#endif
#ifdef HAVE_BOOTFLASH
#include "bootflash.h"
#endif
#ifdef HAVE_BUTTON
#include "button.h"
#endif


struct library_handle* library_list_head;
struct mutex library_mutex;


struct library_handle* library_register(void* image, struct emcorelib_header* header)
{
    mutex_lock(&library_mutex, TIMEOUT_BLOCK);
    struct library_handle* h;
    for (h = library_list_head; h; h = h->next)
        if (h->lib->identifier == header->identifier && h->lib->version == header->version)
        {
            mutex_unlock(&library_mutex);
            return NULL;
        }
    if (header->initfunc && header->initfunc() < 0)
    {
        mutex_unlock(&library_mutex);
        return NULL;
    }
    struct library_handle* handle = (struct library_handle*)malloc(sizeof(struct library_handle));
    memset(handle, 0, sizeof(struct library_handle));
    reownalloc(handle, (struct scheduler_thread*)handle);
    reownalloc(image, (struct scheduler_thread*)handle);
    handle->next = library_list_head;
    handle->lib = header;
    handle->alloc = image;
    library_list_head = handle;
    mutex_unlock(&library_mutex);
    return handle;
}

int library_unload(struct library_handle* lib)
{
    mutex_lock(&library_mutex, TIMEOUT_BLOCK);
    int i;
    bool found = false;
    struct library_handle* h;
    for (h = library_list_head; h; h = h->next)
        if (h == lib)
        {
            found = true;
            break;
        }
    if (!found)
    {
        mutex_unlock(&library_mutex);
        return -1;
    }
    for (i = 0; i < ARRAYLEN(lib->users); i++)
        if (lib->users[i])
        {
            mutex_unlock(&library_mutex);
            return -2;
        }
    if (lib->moreusers)
        for (i = 0; i < lib->moreusers_size / 4; i++)
            if (lib->moreusers[i])
            {
                mutex_unlock(&library_mutex);
                return -2;
            }
    if (lib->lib->shutdownfunc && lib->lib->shutdownfunc() < 0)
    {
        mutex_unlock(&library_mutex);
        return -3;
    }
    if (library_list_head == lib) library_list_head = lib->next;
    else
        for (h = library_list_head; h; h = h->next)
            if (h == lib)
            {
                h->next = lib->next;
                break;
            }
    library_release_all_of_thread((struct scheduler_thread*)lib);
#ifdef HAVE_STORAGE
    close_all_of_process((struct scheduler_thread*)lib);
    closedir_all_of_process((struct scheduler_thread*)lib);
#endif
#ifdef HAVE_BUTTON
    button_unregister_all_of_thread((struct scheduler_thread*)lib);
#endif
    free_all_of_thread((struct scheduler_thread*)lib);
    mutex_unlock(&library_mutex);
    return 0;
}

struct emcorelib_header* get_library_ext(uint32_t identifier, uint32_t minversion,
                                         uint32_t maxversion, enum library_sourcetype sourcetype,
                                         void* source, struct scheduler_thread* owner)
{
    int i;
    int version = minversion - 1;
    struct library_handle* h;
    struct library_handle* best = NULL;
    mutex_lock(&library_mutex, TIMEOUT_BLOCK);
    for (h = library_list_head; h; h = h->next)
        if (h->lib->identifier == identifier &&
            h->lib->version > version && h->lib->version <= maxversion)
        {
            best = h;
            version = h->lib->version;
            break;
        }
    if (!best)
    {
        switch (sourcetype)
        {
        case LIBSOURCE_RAM_ALLOCED:
        {
            best = (struct library_handle*)execimage(source, false);
            break;
        }

        case LIBSOURCE_RAM_NEEDCOPY:
        {
            best = (struct library_handle*)execimage(source, true);
            break;
        }

#ifdef HAVE_BOOTFLASH
        case LIBSOURCE_BOOTFLASH:
        {
            int size = bootflash_filesize((char*)source);
            if (size <= 0) break;
            void* buffer = memalign(0x10, size);
            if (!buffer) break;
            if (bootflash_read((char*)source, buffer, 0, size) != size)
            {
                free(buffer);
                break;
            }
            best = (struct library_handle*)execimage(buffer, false);
            break;
        }
#endif

#ifdef HAVE_STORAGE
        case LIBSOURCE_FILESYSTEM:
        {
            int fd = file_open((char*)source, O_RDONLY);
            if (fd <= 0) break;
            int size = filesize(fd);
            if (size <= 0)
            {
                close(fd);
                break;
            }
            void* buffer = memalign(0x10, size);
            if (!buffer)
            {
                close(fd);
                break;
            }
            if (read(fd, buffer, size) != size)
            {
                free(buffer);
                close(fd);
                break;
            }
            close(fd);
            best = (struct library_handle*)execimage(buffer, false);
            break;
        }
#endif
        }
        if (!best)
        {
            mutex_unlock(&library_mutex);
            return NULL;
        }
    }
    for (i = 0; i < ARRAYLEN(best->users); i++)
        if (best->users[i] == NULL)
        {
            best->users[i] = owner;
            mutex_unlock(&library_mutex);
            return best->lib;
        }
    if (best->moreusers)
        for (i = 0; i < best->moreusers_size / 4; i++)
            if (h->moreusers[i] == NULL)
                {
                    h->moreusers[i] = owner;
                    mutex_unlock(&library_mutex);
                    return best->lib;
                }
    void* newalloc = realloc(best->moreusers, best->moreusers_size + 64);
    if (!newalloc)
    {
        mutex_unlock(&library_mutex);
        return NULL;
    }
    best->moreusers = (void**)newalloc;
    best->moreusers[best->moreusers_size / 4] = owner;
    best->moreusers_size += 64;
    mutex_unlock(&library_mutex);
    return best->lib;
}

struct emcorelib_header* get_library(uint32_t identifier, uint32_t minversion, uint32_t maxversion,
                                     enum library_sourcetype sourcetype, void* source)
{
    return get_library_ext(identifier, minversion, maxversion, sourcetype, source, current_thread);
}

int release_library_ext(struct emcorelib_header* lib, struct scheduler_thread* owner)
{
    int i;
    int rc = -2;
    struct library_handle* h;
    mutex_lock(&library_mutex, TIMEOUT_BLOCK);
    for (h = library_list_head; h; h = h->next)
        if (h->lib == lib)
        {
            rc = -1;
            for (i = 0; i < ARRAYLEN(h->users); i++)
                if (h->users[i] == owner)
                {
                    h->users[i] = NULL;
                    rc = 0;
                    break;
                }
            if (rc && h->moreusers)
                for (i = 0; i < h->moreusers_size / 4; i++)
                    if (h->moreusers[i] == owner)
                        {
                            h->moreusers[i] = NULL;
                            rc = 0;
                            break;
                        }
            break;
        }
    mutex_unlock(&library_mutex);
    return rc;
}

int release_library(struct emcorelib_header* lib)
{
    return release_library_ext(lib, current_thread);
}

int library_release_all_of_thread(struct scheduler_thread* thread)
{
    mutex_lock(&library_mutex, TIMEOUT_BLOCK);
    int i;
    int released = 0;
    struct library_handle* h;
    for (h = library_list_head; h; h = h->next)
    {
        for (i = 0; i < ARRAYLEN(h->users); i++)
            if (h->users[i] == thread)
            {
                h->users[i] = NULL;
                released++;
            }
        if (h->moreusers)
            for (i = 0; i < h->moreusers_size / 4; i++)
                if (h->moreusers[i] == thread)
                    {
                        h->moreusers[i] = NULL;
                        released++;
                    }
    }
    mutex_unlock(&library_mutex);
    return released;
}
