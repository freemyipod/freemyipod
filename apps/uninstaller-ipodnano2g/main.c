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


#include "emcoreapp.h"
#include "libboot.h"


void main(int argc, const char** argv);
EMCORE_APP_HEADER("emCORE uninstaller", main, 127)


struct wakeup eventwakeup;
volatile int button;

#define nor ((uint8_t*)0x24000000)
#define norword ((uint32_t*)0x24000000)


void handler(void* user, enum button_event eventtype, int which, int value)
{
    if (eventtype == BUTTON_PRESS) button |= 1 << which;
    wakeup_signal(&eventwakeup);
}

void* safe_memalign(size_t align, size_t size)
{
    void* addr = memalign(align, size);
    if (!addr) panicf(PANIC_KILLTHREAD, "Out of memory!");
    return addr;
}

uint32_t freeret(uint32_t rc, void* ptr)
{
    free(ptr);
    return rc;
}

uint32_t decryptfw(void* image, uint32_t offset)
{
    uint32_t size = ((uint32_t*)image)[5];
    if (size > 0x800000) return 0;
    hwkeyaes(HWKEYAES_DECRYPT, ((uint32_t*)image)[2], &((uint8_t*)image)[offset], size);
    memcpy(image, &((uint8_t*)image)[offset], size);
    return size;
}

uint32_t getfw(const char* filename, uint32_t* sector, uint32_t* size)
{
    uint32_t i;
    uint32_t* buffer = safe_memalign(0x10, 0x800);
    if (storage_read_sectors_md(0, 0, 1, buffer) != 0) return freeret(1, buffer);
    if (*((uint16_t*)((uint32_t)buffer + 0x1FE)) != 0xAA55) return freeret(1, buffer);
    uint32_t startsector = 0;
    for (i = 0x1C2; i < 0x200; i += 0x10)
        if (((uint8_t*)buffer)[i] == 0)
        {
            startsector = *((uint16_t*)((uint32_t)buffer + i + 4))
                        | (*((uint16_t*)((uint32_t)buffer + i + 6)) << 16);
            break;
        }
    if (startsector == 0) return freeret(1, buffer);
    if (storage_read_sectors_md(0, startsector, 1, buffer) != 0) return freeret(1, buffer);
    if (buffer[0x40] != 0x5B68695D) return freeret(1, buffer);
    if (storage_read_sectors_md(0, startsector + 1 + (buffer[0x41] >> 11), 1, buffer) != 0)
        return freeret(1, buffer);
    for (i = 0; i < 0x1FE; i += 10)
        if (memcmp(&buffer[i], filename, 8) == 0)
        {
            *sector = startsector + (buffer[i + 3] >> 11);
            *size = buffer[i + 4] + 0x800;
            free(buffer);
            return 0;
        }
    return freeret(2, buffer);
}

uint32_t readfw(const char* filename, void** address, uint32_t* size)
{
    uint32_t sector;
    uint32_t rc = getfw(filename, &sector, size);
    if (rc) return rc;
    *address = safe_memalign(0x10, *size);
    if (storage_read_sectors_md(0, sector, ((*size + 0x7FF) >> 11), *address) != 0)
        return freeret(1, *address);
    *size = decryptfw(*address, 0x800);
    *address = realloc(*address, *size);
    return 0;
}

void main(int argc, const char** argv)
{
    uint32_t i, j, k;
    uint8_t* aupd;
    uint32_t aupdsize;
	uint32_t payloadstart = 0;
    struct progressbar_state progressbar;

    cputc(1, '.');

    if (norword[0x400] != 0x53436667)
        panicf(PANIC_KILLTHREAD, "Boot flash contents are damaged! (No SYSCFG found)\n\n"
                                 "Please ask for help.");

    aupdsize = 0;
    if (readfw("DNANdpua", (void**)&aupd, &aupdsize)) aupdsize = 0;
    cputc(1, '.');
    if (!aupdsize)
    {
        cputs(3, "\n\nPlease press any key to\nenter disk mode and\nrestore your iPod using\n"
                 "iTunes. After your iPod\nreboots, run the\nuninstaller again.\n"
                 "If this message is still\nshown after restoring,\nplease ask for help.");
        wakeup_init(&eventwakeup);
        struct button_hook_entry* hook = button_register_handler(handler, NULL);
        if (!hook) panicf(PANIC_KILLTHREAD, "Could not register button hook!");
        button = 0;
        while (true)
        {
            wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
            if (button)
            {
                void* firmware = NULL;
                int size;
                struct emcorelib_header* lib = get_library(LIBBOOT_IDENTIFIER, LIBBOOT_API_VERSION,
                                                           LIBSOURCE_BOOTFLASH, "libboot ");
                if (!lib) panicf(PANIC_KILLTHREAD, "Could not load libboot!");
                struct libboot_api* boot = (struct libboot_api*)lib->api;
                boot->load_from_flash(&firmware, &size, false, "diskmode", 0x100000);
                release_library(lib);
                library_unload(lib);
                if (!firmware)
                    panicf(PANIC_KILLTHREAD, "Could not boot disk mode.\nPlease ask for help.");
                shutdown(false);
                execfirmware((void*)0x08000000, firmware, size);
            }
        }
    }

    cputc(1, '.');
    uint8_t* norbuf = (uint8_t*)safe_memalign(0x10, 0x100000);
    #define norbufword ((uint32_t*)norbuf)
    cputc(1, '.');
    memset(norbuf, 0xff, 0x100000);
    cputc(1, '.');
    memcpy(&norbuf[0x4000], &nor[0x1000], 0x1000);
    cputc(1, '.');
    
    for (i = 0; i < (aupdsize >> 2); i++)
		if (((uint32_t*)aupd)[i] == 0x50796c64 && ((uint32_t*)aupd)[i + 4] == 0x46775570)
		    payloadstart = i << 2;
	if (!payloadstart)
        panicf(PANIC_KILLTHREAD, "Restore image is weird:\nNo payload start found.\n"
                                 "Please ask for help.");
    cputc(1, '.');
	memcpy(norbuf, &aupd[payloadstart + 0x2c], 0x2000);
    cputc(1, '.');
	memcpy(&norbuf[0x8800], &aupd[payloadstart + 0x2048], 0x1f800);
    cputc(1, '.');
	memcpy(&norbuf[0x28000], &aupd[payloadstart + 0x22048], 0xd8000);
    cputc(1, '.');
	free(aupd);

    memset(&norbuf[0x8000], 0, 0x800);
    cputc(1, '.');
    norbufword[0x2000] = 0x31303738;
    norbufword[0x2001] = 0x00302e31;
    norbufword[0x2002] = 0x800;
    norbufword[0x2003] = 0x1f800;
    cputc(1, '.');
    hmacsha1(&norbuf[0x8800], 0x1f800, &norbuf[0x8010]);
    cputc(1, '.');
    hmacsha1(&norbuf[0x8000], 0x40, &norbuf[0x8040]);
    cputc(1, '.');
    hwkeyaes(HWKEYAES_ENCRYPT, 2, &norbuf[0x8000], 0x20000);
    cputc(1, '.');
    for (i = 0xffe00; i < 0xffec8; i += 0x28)
        if (norbufword[i >> 2])
        {
            uint32_t offs = norbufword[(i >> 2) + 3] >> 2;
            norbufword[offs] = 0;
            norbufword[offs + 1] = 2;
            norbufword[offs + 2] = 2;
            norbufword[offs + 3] = 0x40;
            norbufword[offs + 4] = 0;
            norbufword[offs + 5] = norbufword[(i >> 2) + 4];
            cputc(1, '.');
            hmacsha1(&norbufword[offs + 0x80], norbufword[(i >> 2) + 4], &norbufword[offs + 7]);
            cputc(1, '.');
            memset(&norbufword[offs + 0x75], 0, 0x14);
            cputc(1, '.');
            hmacsha1(&norbufword[offs], 0x200, &norbufword[offs + 0x75]);
            cputc(1, '.');
            hwkeyaes(HWKEYAES_ENCRYPT, 2, &norbufword[offs + 0x80], norbufword[(i >> 2) + 4]);
            cputc(1, '.');
        }

    for (i = 0; i < 16; i++)
    {
        bootflash_writeraw(&norbuf[i * 0x10000], i * 0x10000, 0x10000);
        cputc(1, '.');
    }
    free(norbuf);
    shutdown(false);
    reset();
}
