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
#include "power.h"
#include "interrupt.h"
#include "util.h"
#include "targetinit.h"
#ifdef HAVE_LCD
#include "lcd.h"
#include "lcdconsole.h"
#endif
#ifdef HAVE_I2C
#include "i2c.h"
#endif
#include "usb/usb.h"
#ifdef HAVE_BACKLIGHT
#include "backlight.h"
#endif
#include "ramdisk.h"
#include "timer.h"


static const char welcomestring[] INITCONST_ATTR = "UMSboot v" VERSION " r" VERSION_SVN "\n\n"
                                                   "Please copy a UBI file to\n"
                                                   "the mass storage device and\n"
                                                   "safely eject and unplug it\n"
                                                   "when you're done.\n\n"
                                                   "If you booted this\n"
                                                   "accidentally, just press and\n"
                                                   "hold MENU+SELECT to reboot.\n";

static uint16_t swapmap[RAMDISK_SECTORS];
static uint16_t swaprev[RAMDISK_SECTORS];
static char swapbuf[RAMDISK_SECTORSIZE];
static uint16_t newfat[RAMDISK_SECTORS];
static char newdir[RAMDISK_SECTORSIZE];


extern void* _ramdiskptr;


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
    memset(buffer, 0, 0x800);
    memcpy(buffer, "\xeb\x58\x00MSWIN5.0\0\x08", 0xd);
    buffer[0xd] = 1;
    ((uint16_t*)buffer)[7] = 1;
    memcpy(&buffer[0x10], "\x01\x40\0\0\x40\xf8\0\0\x3f\0\xff", 0xb);
    buffer[0x13] = sectors & 0xff;
    buffer[0x14] = sectors >> 8;
    ((uint16_t*)buffer)[0xb] = fatsectors;
    buffer[0x24] = 0x80;
    buffer[0x26] = 0x29;
    memcpy(&buffer[0x27], "UBRDUMSboot    FAT16   ", 0x17);
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
    uint16_t srcmap = swapmap[src];
    uint16_t destmap = swaprev[dest];
    memcpy(swapbuf, ramdisk[dest], RAMDISK_SECTORSIZE);
    memcpy(ramdisk[dest], ramdisk[srcmap], RAMDISK_SECTORSIZE);
    memcpy(ramdisk[srcmap], swapbuf, RAMDISK_SECTORSIZE);
    swapmap[src] = dest;
    swapmap[destmap] = srcmap;
    swaprev[srcmap] = destmap;
    swaprev[dest] = src;
}

void main()
{
    int i;
    int fatsectors = fat16_write_mbr(ramdisk[0], RAMDISK_SECTORS);
    fat16_write_fat(ramdisk[1], fatsectors);
    fat16_write_rootdir(ramdisk[1 + fatsectors]);
    usb_init();
    for (i = 0; i < RAMDISK_SECTORS; i++) swapmap[i] = i;
    memcpy(swaprev, swapmap, sizeof(swaprev));
    while (!vbus_state());
    udelay(1000000);
    while (vbus_state() && !usb_ejected);
    udelay(200000);
    usb_exit();
    lcdconsole_puts("\nLoading UBI File...\n", 0, 0xffff);
    int found = 0;
    uint16_t cluster;
    uint32_t size;
    uint16_t totalclusters = 0;
    for (i = 0; i < RAMDISK_SECTORSIZE; i += 32)
        if (!ramdisk[1 + fatsectors][i]) break;
        else if (ramdisk[1 + fatsectors][i] == 0xe5) continue;
        else if (((*((uint32_t*)&ramdisk[1 + fatsectors][i + 8])) & 0xffffff) == 0x494255)
        {
            cluster = *((uint16_t*)&ramdisk[1 + fatsectors][i + 26]);
            size = *((uint32_t*)&ramdisk[1 + fatsectors][i + 28]);
            ramdisk[1 + fatsectors][i] = 0xe5;
            found++;
        }
        else totalclusters += (*((uint32_t*)&ramdisk[1 + fatsectors][i + 28])
                             + RAMDISK_SECTORSIZE - 1) / RAMDISK_SECTORSIZE;
    if (!found)
    {
        lcdconsole_puts("No UBI file found!", 0, 0xffff);
        return;
    }
    if (found != 1)
    {
        lcdconsole_puts("Multiple UBI files found!\nPlease copy exactly one.", 0, 0xffff);
        return;
    }
    if (!size || !cluster)
    {
        lcdconsole_puts("UBI file is empty!", 0, 0xffff);
        return;
    }
    uint16_t dest = 0;
    while (cluster != 0xffff)
    {
        swap(fatsectors + cluster, dest++);
        cluster = *((uint16_t*)&ramdisk[swapmap[1 + (cluster / (RAMDISK_SECTORSIZE / 2))]]
                                       [(cluster % (RAMDISK_SECTORSIZE / 2)) * 2]); 
    }
    lcdconsole_puts("Rearranging files...\n", 0, 0xffff);
    uint16_t offset = RAMDISK_SECTORS - totalclusters - 2;
    memset(newfat, 0, sizeof(newfat));
    memset(newdir, 0, sizeof(newdir));
    *((uint32_t*)newfat) = *((uint32_t*)ramdisk[swapmap[1]]);
    dest = 2;
    int newptr = 0;
    for (i = 0; i < RAMDISK_SECTORSIZE; i += 32)
        if (!ramdisk[swapmap[1 + fatsectors]][i]) break;
        else if (ramdisk[swapmap[1 + fatsectors]][i] == 0xe5) continue;
        else
        {
            memcpy(&newdir[newptr], &ramdisk[swapmap[1 + fatsectors]][i], 0x20);
            cluster = *((uint16_t*)&newdir[newptr + 26]);
            size = *((uint32_t*)&newdir[newptr + 28]);
            if (cluster)
            {
                *((uint16_t*)&newdir[newptr + 26]) = dest;
                while (cluster != 0xffff)
                {
                    swap(fatsectors + cluster, offset + dest++);
                    cluster = *((uint16_t*)&ramdisk[swapmap[1 + (cluster / (RAMDISK_SECTORSIZE / 2))]]
                                                   [(cluster % (RAMDISK_SECTORSIZE / 2)) * 2]);
                    if (cluster == 0xffff) newfat[dest - 1] = 0xffff;
                    else newfat[dest - 1] = dest;
                }
            }
            newptr += 0x20;
        }
    fatsectors = (2 * (totalclusters + 2) + RAMDISK_SECTORSIZE - 1) / RAMDISK_SECTORSIZE;
    _ramdiskptr = ramdisk[offset - fatsectors];
    memcpy(ramdisk[offset + 1], newdir, RAMDISK_SECTORSIZE);
    memcpy(ramdisk[offset - fatsectors + 1], newfat, fatsectors * RAMDISK_SECTORSIZE);
    fat16_write_mbr(_ramdiskptr, totalclusters + fatsectors + 2);
    lcdconsole_puts("Booting...", 0, 0xffff);
    execfirmware(ramdisk[0]);
}

void init() INITCODE_ATTR;
void init()
{
#ifdef HAVE_TARGETINIT_VERYEARLY
    targetinit_veryearly();
#endif
#ifdef HAVE_LCD
    lcd_init();
    lcdconsole_init();
#endif
#ifdef HAVE_TARGETINIT_EARLY
    targetinit_early();
#endif
    interrupt_init();
#ifdef HAVE_I2C
    i2c_init();
#endif
    power_init();
    lcdconsole_puts(welcomestring, 0, 0xffff);
#ifdef HAVE_BACKLIGHT
    backlight_init();
#endif
#ifdef HAVE_TARGETINIT_LATE
    targetinit_late();
#endif
}
