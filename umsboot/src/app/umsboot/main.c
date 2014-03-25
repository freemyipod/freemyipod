#include "global.h"
#include "app/umsboot/main.h"
#include "app/main.h"
#include "app/umsboot/usbglue.h"
#include "app/umsboot/ums.h"
#include "sys/time.h"
#include "sys/util.h"
#include "protocol/usb/usb.h"

#ifdef UMSBOOT_HAVE_CONSOLE
#include "interface/console/console.h"
#include "app/umsboot/console.h"
#endif


static uint16_t swapmap[RAMDISK_SECTORS] __attribute__((aligned(CACHEALIGN_SIZE)));
static uint16_t swaprev[RAMDISK_SECTORS] __attribute__((aligned(CACHEALIGN_SIZE)));
static char swapbuf[RAMDISK_SECTORSIZE] __attribute__((aligned(CACHEALIGN_SIZE)));
static uint16_t newfat[RAMDISK_SECTORS] __attribute__((aligned(CACHEALIGN_SIZE)));
static char newdir[RAMDISK_SECTORSIZE] __attribute__((aligned(CACHEALIGN_SIZE)));
static int fatsectors;


int fat16_calc_fatsectors(int sectors)
{
    uint32_t fatsectors = 1;
    uint32_t oldfatsectors = 0;
    uint32_t clustercount;
    while (fatsectors != oldfatsectors)
    {
        oldfatsectors = fatsectors;
        clustercount = sectors - fatsectors - 2;
        fatsectors = (2 * (clustercount + 2) + RAMDISK_SECTORSIZE - 1) / RAMDISK_SECTORSIZE;
    }
    return fatsectors;
}

int fat16_write_mbr(uint8_t* buffer, int sectors)
{
    uint32_t fatsectors = fat16_calc_fatsectors(sectors);
    memset(buffer, 0, RAMDISK_SECTORSIZE);
    memcpy(buffer, "\xeb\x58\x00MSWIN5.0", 0xb);
    buffer[0xb] = RAMDISK_SECTORSIZE & 0xff;
    buffer[0xc] = RAMDISK_SECTORSIZE >> 8;
    buffer[0xd] = 1;
    ((uint16_t*)buffer)[7] = 1;
    buffer[0x10] = 1;
    buffer[0x11] = (RAMDISK_SECTORSIZE >> 5) & 0xff;
    buffer[0x12] = RAMDISK_SECTORSIZE >> 13;
    buffer[0x13] = sectors & 0xff;
    buffer[0x14] = sectors >> 8;
    buffer[0x15] = 0xf8;
    buffer[0x18] = 0x3f;
    buffer[0x1a] = 0xff;
    ((uint16_t*)buffer)[0xb] = fatsectors;
    memcpy(&buffer[0x24], "\x80\0\x29UBRDUMSboot    FAT16   ", 0x1a);
    ((uint16_t*)buffer)[0xff] = 0xaa55;
    return fatsectors;
}

void fat16_write_fat(uint8_t* buffer, int sectors)
{
    memset(buffer, 0, sectors * RAMDISK_SECTORSIZE);
    *((uint32_t*)buffer) = 0xfffffff8;
}

void fat16_write_rootdir(uint8_t* buffer)
{
    memset(buffer, 0, RAMDISK_SECTORSIZE);
    memcpy(buffer, "UMSboot    \x08", 12);
}

void swap(src, dest)
{
    memcpy(swapbuf, RAMDISK[dest], RAMDISK_SECTORSIZE);
    uint16_t srcmap = swapmap[src];
    uint16_t destmap = swaprev[dest];
    memcpy(RAMDISK[dest], RAMDISK[srcmap], RAMDISK_SECTORSIZE);
    swapmap[src] = dest;
    swaprev[dest] = src;
    memcpy(RAMDISK[srcmap], swapbuf, RAMDISK_SECTORSIZE);
    swaprev[srcmap] = destmap;
    swapmap[destmap] = srcmap;
}

int main()
{
    int i;
#ifdef UMSBOOT_HAVE_CONSOLE
    umsboot_console_init();
#endif
    fatsectors = fat16_write_mbr(RAMDISK[0], RAMDISK_SECTORS);
    fat16_write_fat(RAMDISK[1], fatsectors);
    fat16_write_rootdir(RAMDISK[1 + fatsectors]);
    for (i = 0; i < RAMDISK_SECTORS; i++) swapmap[i] = i;
    memcpy(swaprev, swapmap, sizeof(swaprev));
    while (true)
    {
        ums_ejected = false;
        usb_init(&usb_data);
        while (!ums_ejected) idle();
        udelay(100000);
        usb_exit(&usb_data);
#ifdef UMSBOOT_HAVE_CONSOLE
        console_puts(&umsboot_console, "\nLoading UBI file...\n");
        console_flush(&umsboot_console);
#endif
        int found = 0;
        uint16_t ubicluster = 0;
        uint32_t ubisize = 0;
        uint16_t cluster = 0;
        uint16_t totalclusters = 0;
        for (i = 0; i < RAMDISK_SECTORSIZE; i += 32)
            if (!RAMDISK[1 + fatsectors][i]) break;
            else if (RAMDISK[1 + fatsectors][i] == 0xe5) continue;
            else if (((((uint32_t*)RAMDISK[1 + fatsectors])[(i + 8) >> 2]) & 0xffffff) == 0x494255)
            {
                ubicluster = ((uint16_t*)RAMDISK[1 + fatsectors])[(i + 26) >> 1];
                ubisize = ((uint32_t*)RAMDISK[1 + fatsectors])[(i + 28) >> 2];
                RAMDISK[1 + fatsectors][i] = 0xe5;
                found++;
            }
            else if ((cluster = ((uint16_t*)RAMDISK[1 + fatsectors])[(i + 26) >> 1]))
                while (cluster != 0xffff)
                {
                    cluster = ((uint16_t*)RAMDISK[1])[cluster];
                    totalclusters++;
                }
        if (!found)
        {
#ifdef UMSBOOT_HAVE_CONSOLE
            console_puts(&umsboot_console, "No UBI file found!\nPlease retry.\n");
            console_flush(&umsboot_console);
#endif
            continue;
        }
        if (found != 1)
        {
#ifdef UMSBOOT_HAVE_CONSOLE
            console_puts(&umsboot_console, "Multiple UBI files found!\nPlease copy exactly one.\nPlease retry.\n");
            console_flush(&umsboot_console);
#endif
            continue;
        }
        if (!ubisize || !ubicluster)
        {
#ifdef UMSBOOT_HAVE_CONSOLE
            console_puts(&umsboot_console, "UBI file is empty!\nPlease retry.\n");
            console_flush(&umsboot_console);
#endif
            continue;
        }
        uint16_t dest = 0;
        while (ubicluster != 0xffff)
        {
            swap(fatsectors + ubicluster, dest++);
            ubicluster = ((uint16_t*)RAMDISK[swapmap[1 + (ubicluster / (RAMDISK_SECTORSIZE / 2))]])
                                            [ubicluster % (RAMDISK_SECTORSIZE / 2)];
        }
#ifdef UMSBOOT_HAVE_CONSOLE
        console_puts(&umsboot_console, "Rearranging files...\n");
        console_flush(&umsboot_console);
#endif
        uint16_t offset = RAMDISK_SECTORS - totalclusters - 2;
        memset(newfat, 0, sizeof(newfat));
        memset(newdir, 0, sizeof(newdir));
        newfat[0] = ((uint16_t*)RAMDISK[swapmap[1]])[0];
        newfat[1] = ((uint16_t*)RAMDISK[swapmap[1]])[1];
        dest = 2;
        int newptr = 0;
        for (i = 0; i < RAMDISK_SECTORSIZE; i += 32)
            if (!RAMDISK[swapmap[1 + fatsectors]][i]) break;
            else if (RAMDISK[swapmap[1 + fatsectors]][i] == 0xe5) continue;
            else
            {
                memcpy(&newdir[newptr], &RAMDISK[swapmap[1 + fatsectors]][i], 0x20);
                cluster = ((uint16_t*)newdir)[(newptr + 26) >> 1];
                if (cluster)
                {
                    ((uint16_t*)newdir)[(newptr + 26) >> 1] = dest;
                    while (cluster != 0xffff)
                    {
                        swap(fatsectors + cluster, offset + dest++);
                        cluster = ((uint16_t*)RAMDISK[swapmap[1 + (cluster / (RAMDISK_SECTORSIZE / 2))]])
                                                     [cluster % (RAMDISK_SECTORSIZE / 2)];
                        if (cluster == 0xffff) newfat[dest - 1] = 0xffff;
                        else newfat[dest - 1] = dest;
                    }
                }
                newptr += 0x20;
            }
        fatsectors = (2 * (totalclusters + 2) + RAMDISK_SECTORSIZE - 1) / RAMDISK_SECTORSIZE;
        RAMDISK_PTR = RAMDISK[offset - fatsectors];
        memcpy(RAMDISK[offset + 1], newdir, RAMDISK_SECTORSIZE);
        memcpy(RAMDISK[offset - fatsectors + 1], newfat, fatsectors * RAMDISK_SECTORSIZE);
        fat16_write_mbr(RAMDISK_PTR, totalclusters + fatsectors + 2);
#ifdef UMSBOOT_HAVE_CONSOLE
        console_puts(&umsboot_console, "Booting UBI file...");
        console_flush(&umsboot_console);
#endif
#ifdef DEBUG
        continue;
#else
        execfirmware(RAMDISK[0]);
#endif
    }
}
