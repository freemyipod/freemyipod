#include "embiosapp.h"


void main();
EMBIOS_APP_HEADER("Uninstaller thread", 0x1000, main, 127)


uint16_t lcdbuffer[176 * 132];

struct wakeup eventwakeup;
volatile int button;

char mallocbuf[0x1000000] __attribute__((aligned(16)));
tlsf_pool mallocpool;

uint8_t norbuf[0x100000] __attribute__((aligned(16)));
#define norbufword ((uint32_t*)norbuf)

#define nor ((uint8_t*)0x24000000)
#define norword ((uint32_t*)0x24000000)


void handler(enum button_event eventtype, int which, int value)
{
    if (eventtype == BUTTON_PRESS) button |= 1 << which;
    wakeup_signal(&eventwakeup);
}

uint32_t freeret(uint32_t rc, void* ptr)
{
    tlsf_free(mallocpool, ptr);
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
    uint32_t* buffer = tlsf_memalign(mallocpool, 0x10, 0x800);
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
            tlsf_free(mallocpool, buffer);
            return 0;
        }
    return freeret(2, buffer);
}

uint32_t readfwptr(const char* filename, void* address, uint32_t* size)
{
    uint32_t sector;
    uint32_t rc = getfw(filename, &sector, size);
    if (rc) return rc;
    if (storage_read_sectors_md(0, sector, ((*size + 0x7FF) >> 11), address) != 0) return 1;
    *size = decryptfw(address, 0x800);
    return 0;
}

uint32_t readfw(const char* filename, void** address, uint32_t* size)
{
    uint32_t sector;
    uint32_t rc = getfw(filename, &sector, size);
    if (rc) return rc;
    *address = tlsf_memalign(mallocpool, 0x10, *size);
    if (storage_read_sectors_md(0, sector, ((*size + 0x7FF) >> 11), *address) != 0)
        return freeret(1, *address);
    *size = decryptfw(*address, 0x800);
    tlsf_realloc(mallocpool, *address, *size);
    return 0;
}

void main(void)
{
    uint32_t i, j, k;
    uint8_t* aupd;
    uint32_t aupdsize;
	uint32_t payloadstart = 0;
    struct progressbar_state progressbar;

    wakeup_init(&eventwakeup);
    button_register_handler(handler);
    mallocpool = tlsf_create(mallocbuf, sizeof(mallocbuf));

    memset(lcdbuffer, 0xff, 176 * 132 * 2);
    rendertext(&lcdbuffer[177], 0, 0xffff, "Loading...", 176);
    displaylcd(0, 175, 0, 131, lcdbuffer, 0);
    if (norword[0x400] != 0x53436667)
    {
        cputs(1, "Boot flash contents are damaged! (No SYSCFG found)\n\nPlease ask for help.\n");
        return;
    }

    memset(norbuf, 0xff, 0x100000);
    memcpy(&norbuf[0x4000], &nor[0x1000], 0x1000);

    aupdsize = 0;
    if (readfw("DNANdpua", (void**)&aupd, &aupdsize)) aupdsize = 0;
    if (!aupdsize)
    {
        memset(lcdbuffer, 0xff, 176 * 132 * 2);
        rendertext(&lcdbuffer[176 * 10 + 10], 0, 0xffff, "Please press any key to", 176);
        rendertext(&lcdbuffer[176 * 18 + 10], 0, 0xffff, "enter disk mode and", 176);
        rendertext(&lcdbuffer[176 * 26 + 10], 0, 0xffff, "restore your iPod using", 176);
        rendertext(&lcdbuffer[176 * 34 + 10], 0, 0xffff, "iTunes. Your iPod will", 176);
        rendertext(&lcdbuffer[176 * 42 + 10], 0, 0xffff, "reboot and ask you to", 176);
        rendertext(&lcdbuffer[176 * 50 + 10], 0, 0xffff, "uninstall iLoader again.", 176);
        rendertext(&lcdbuffer[176 * 66 + 10], 0, 0xffff, "If this message is still", 176);
        rendertext(&lcdbuffer[176 * 74 + 10], 0, 0xffff, "shown after restoring,", 176);
        rendertext(&lcdbuffer[176 * 82 + 10], 0, 0xffff, "please ask for help.", 176);
        displaylcd(0, 175, 0, 131, lcdbuffer, 0);
        button = 0;
        while (true)
        {
            wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
            if (button)
            {
                int size = bootflash_filesize("diskmode");
                cprintf(1, "\nsize = %d bytes\n", size);
                if (size > 0)
                {
                    if (bootflash_attributes("diskmode") & 0x800)
                    {
                        if (!ucl_decompress(bootflash_getaddr("diskmode"), size,
                                            (void*)0x08000000, (uint32_t*)&size))
                        {
                            shutdown(false);
                            execfirmware((void*)0x08000000);
                        }
                    }
                    else if (bootflash_read("diskmode", (void*)0x08000000, 0, size) == size)
                    {
                        shutdown(false);
                        execfirmware((void*)0x08000000);
                    }
                }
                cputs(1, "Could not boot disk mode.\nPlease ask for help.\n");
                return;
            }
        }
    }
	for (i = 0; i < (aupdsize >> 2); i++)
		if (((uint32_t*)aupd)[i] == 0x50796c64 && ((uint32_t*)aupd)[i + 4] == 0x46775570)
		    payloadstart = i << 2;
	if (!payloadstart)
    {
        cputs(1, "Restore image is weird:\nNo payload start found.\nPlease ask for help.\n");
        return;
    }
	memcpy(norbuf, &aupd[payloadstart + 0x2c], 0x2000);
	memcpy(&norbuf[0x8800], &aupd[payloadstart + 0x2048], 0x1f800);
	memcpy(&norbuf[0x28000], &aupd[payloadstart + 0x22048], 0xd8000);
	tlsf_free(mallocpool, aupd);

    rendertext(&lcdbuffer[177], 0, 0xffff, "Encrypting flash update...", 176);
    displaylcd(0, 175, 0, 131, lcdbuffer, 0);
    memset(&norbuf[0x8000], 0, 0x800);
    norbufword[0x2000] = 0x31303738;
    norbufword[0x2001] = 0x00302e31;
    norbufword[0x2002] = 0x800;
    norbufword[0x2003] = 0x1f800;
    hmacsha1(&norbuf[0x8800], 0x1f800, &norbuf[0x8010]);
    hmacsha1(&norbuf[0x8000], 0x40, &norbuf[0x8040]);
    hwkeyaes(HWKEYAES_ENCRYPT, 2, &norbuf[0x8000], 0x20000);
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
            hmacsha1(&norbufword[offs + 0x80], norbufword[(i >> 2) + 4], &norbufword[offs + 7]);
            memset(&norbufword[offs + 0x75], 0, 0x14);
            hmacsha1(&norbufword[offs], 0x200, &norbufword[offs + 0x75]);
            hwkeyaes(HWKEYAES_ENCRYPT, 2, &norbufword[offs + 0x80], norbufword[(i >> 2) + 4]);
        }

    rendertext(&lcdbuffer[177], 0, 0xffff, "Flashing... (DO NOT RESET!!!)", 176);
    displaylcd(0, 175, 0, 131, lcdbuffer, 0);
    progressbar_init(&progressbar, 1, 174, 10, 17, 0, lcd_translate_color(0, 0xcf, 0xcf, 0xcf),
                     lcd_translate_color(0, 0, 0, 0xcf), 0, 256);
    for (i = 0; i < 256; i++)
    {
        bootflash_writeraw(&norbuf[i * 0x1000], i * 0x1000, 0x1000);
        progressbar_setpos(&progressbar, i, false);
    }
    rendertext(&lcdbuffer[177], 0, 0xffff, "Uninstallation successful!   ", 176);
    displaylcd(0, 175, 0, 131, lcdbuffer, 0);
    sleep(1000000);
    shutdown(true);
    reset();
}
