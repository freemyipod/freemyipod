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
#include "console.h"
#include "execimage.h"
#include "mmu.h"


struct scheduler_thread* execimage(void* image, bool copy)
{
    struct emcoreapp_header* header = (struct emcoreapp_header*)image;
    if (memcmp(header, "emCOexec", 8))
    {
        cprintf(CONSOLE_BOOT, "execimage: Bad signature: "
                              "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                header->signature[0], header->signature[1], header->signature[2],
                header->signature[3], header->signature[4], header->signature[5],
                header->signature[6], header->signature[7]);
        return NULL;
    }
    if (header->version != EMCOREAPP_HEADER_VERSION)
    {
        cprintf(CONSOLE_BOOT, "execimage: Unsupported version! (%08X)\n", header->version);
        return NULL;
    }
    off_t offset = header->textstart;
    size_t textsize = header->textsize;
    size_t bsssize = header->bsssize;
    size_t stacksize = header->stacksize;
	off_t entrypoint = header->entrypoint;
	off_t relocstart = header->relocstart - offset;
	uint32_t reloccount = header->reloccount;
	bool compressed = header->flags & EMCOREAPP_FLAG_COMPRESSED;
    size_t finalsize = textsize + bsssize + stacksize;
    size_t datasize = relocstart + reloccount * 4;
    size_t tempsize = MAX(finalsize, datasize);
    if (compressed)
    {
        void* alloc = malloc(tempsize);
        if (!alloc)
        {
            cprintf(CONSOLE_BOOT, "execimage: Out of memory!\n");
            return NULL;
        }
        uint32_t decompsize;
        if (ucl_decompress(image + offset, datasize, alloc, &decompsize))
        {
            cprintf(CONSOLE_BOOT, "execimage: Decompression failed!\n");
            free(alloc);
            return NULL;
        }
        if (datasize != decompsize)
        {
            cprintf(CONSOLE_BOOT, "execimage: Decompressed size mismatch!\n");
            free(alloc);
            return NULL;
        }
        if (!copy) free(image);
        image = alloc;
    }
    else if (copy)
    {
        void* alloc = malloc(tempsize);
        if (!alloc)
        {
            cprintf(CONSOLE_BOOT, "execimage: Out of memory!\n");
            return NULL;
        }
        memcpy(alloc, image + offset, datasize);
        image = alloc;
    }
    else
    {
        memcpy(image, image + offset, datasize);
        image = realloc(image, tempsize);
        if (!image)
        {
            cprintf(CONSOLE_BOOT, "execimage: Out of memory!\n");
            return NULL;
        }
    }
    for (; reloccount; reloccount--, relocstart += 4)
    {
        off_t reloc = *((off_t*)(image + relocstart));
        uint32_t data = *((uint32_t*)(image + reloc));
        *((void**)(image + reloc)) = image + data;
    }
    if (tempsize != finalsize) realloc(image, finalsize); /* Can only shrink => safe */
    clean_dcache();
    invalidate_icache();
    struct scheduler_thread* thread = thread_create(NULL, NULL, image + entrypoint,
                                                    image + textsize + bsssize, stacksize,
                                                    USER_THREAD, 127, false);
    reownalloc(image, thread);
    thread_resume(thread);
    return thread;
}
