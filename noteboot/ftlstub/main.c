//
//
//    Copyright 2009 TheSeven
//
//
//    This file is part of the Linux4Nano toolkit.
//
//    TheSeven's iBugger is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    TheSeven's iBugger is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with the Linux4Nano toolkit.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include <toolkit.h>
#include <ftl.h>
#include "timer.h"

extern void _loaddest() __attribute__((noreturn));
extern char _filename;

uint32_t fatsect[0x2000] __attribute__((aligned(16)));

void main()
{
    int i;
    singlebeep(50, 100000);
    sleep(50000);
    singlebeep(40, 100000);
    if (ftl_init()) beep(2);
    ftl_read(0, 1, fatsect);
    if (((uint16_t*)fatsect)[0xff] != 0xaa55) beep(5);
    uint32_t partstart = 0;
    for (i = 0x1c2; i < 0x200; i += 0x10)
        if (((uint8_t*)fatsect)[i] == 0xb)
        {
            partstart = *((uint16_t*)((uint32_t)fatsect + i + 4))
                      | (*((uint16_t*)((uint32_t)fatsect + i + 6)) << 16);
            break;
        }
    if (*((uint16_t*)((uint32_t)fatsect + 0x52)) != 0x4146
     || *((uint8_t*)((uint32_t)fatsect + 0x54)) != 0x54)
        beep(9);
    ftl_read(partstart, 1, fatsect);
    if (((uint16_t*)fatsect)[0xff] != 0xaa55) beep(4);
    uint32_t secperclus = ((uint8_t*)fatsect)[0xd];
    uint32_t reserved = ((uint16_t*)fatsect)[7];
    uint32_t fatcount = ((uint8_t*)fatsect)[0x10];
    uint32_t fatsize = fatsect[9];
    uint32_t rootdirclus = fatsect[0xb];
    uint32_t startsect = partstart + reserved + fatcount * fatsize - 2 * secperclus;
    while (rootdirclus < 0xfffffff)
    {
        ftl_read(startsect + rootdirclus * secperclus, secperclus, fatsect);
        for (i = 0; i < secperclus * 64; i++)
            if (!memcmp(&fatsect[i * 8], "NOTES      ", 11))
            {
                rootdirclus = (((uint16_t*)fatsect)[i * 16 + 0xa] << 16)
                            | ((uint16_t*)fatsect)[i * 16 + 0xd];
                while (rootdirclus < 0xfffffff)
                {
                    ftl_read(startsect + rootdirclus * secperclus, secperclus, fatsect);
                    for (i = 0; i < secperclus * 64; i++)
                        if (!memcmp(&fatsect[i * 8], &_filename, 11))
                        {
                            rootdirclus = (((uint16_t*)fatsect)[i * 16 + 0xa] << 16)
                                        | ((uint16_t*)fatsect)[i * 16 + 0xd];
                            uint32_t dest = (uint32_t)_loaddest;
                            int fatsectnum = -1;
                            while (rootdirclus < 0xfffffff)
                            {
                                ftl_read(startsect + rootdirclus * secperclus, secperclus, (void*)dest);
                                dest += secperclus * 2048;
                                if ((rootdirclus >> 9) != fatsectnum)
                                {
                                    ftl_read(partstart + reserved + (rootdirclus >> 9), 1, fatsect);
                                    fatsectnum = rootdirclus >> 9;
                                }
                                rootdirclus = fatsect[rootdirclus & 0x1ff];
                            }
                            memcpy(_loaddest, (void*)(((uint32_t)_loaddest) + 0x1000), dest - ((uint32_t)_loaddest));
                            singlebeep(40, 100000);
                            sleep(50000);
                            singlebeep(30, 100000);
                            DMACON0 = 0;
                            DMACON1 = 0;
                            DMACON2 = 0;
                            DMACON3 = 0;
                            DMACON4 = 0;
                            DMACON5 = 0;
                            DMACON6 = 0;
                            DMACON7 = 0;
                            DMACON8 = 0;
                            TACMD = 2;
                            TBCMD = 2;
                            TCCMD = 2;
                            TDCMD = 2;
                            TACON = TACON;
                            TBCON = TBCON;
                            TCCON = TCCON;
                            TDCON = TDCON;
                            SRCPND = SRCPND;
                            _loaddest();
                            return;
                        }
                    ftl_read(partstart + reserved + (rootdirclus >> 9), 1, fatsect);
                    rootdirclus = fatsect[rootdirclus & 0x1ff];
                }
                beep(7);
            }
        ftl_read(partstart + reserved + (rootdirclus >> 9), 1, fatsect);
        rootdirclus = fatsect[rootdirclus & 0x1ff];
    }
    beep(6);
}
