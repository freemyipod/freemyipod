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
#include "clickwheel.h"


#define nor ((uint8_t*)0x24000000)
#define norword ((uint32_t*)0x24000000)
#define sysi ((uint8_t*)0x2202bdf0)
#define sysiword ((uint32_t*)0x2202bdf0)


void targetinit_late()
{
    int i;

    clickwheel_init();

    uint32_t scfg_size = norword[0x401];
    uint32_t scfg_entrycount = norword[0x405];
    if (norword[0x400] == 0x53436667 && scfg_size <= 0x1000
     && scfg_entrycount * 0x14 + 0x18 == scfg_size)
    {
        memset(sysi, 0, 0x104);
        sysiword[0] = 0x53797349;
        sysiword[1] = 0x104;
        sysiword[0x21] = 0x100000;
        for (i = 0; i < scfg_entrycount; i++)
            switch (norword[0x406 + i * 5])
            {
                case 0x48774e6d: // HwNm
                    memcpy(&sysi[0x08], &norword[0x406 + i * 5 + 1], 16);
                    break;
                case 0x48775672: // HwVr
                    if (norword[0x406 + i * 5 + 1] == 0)
                        sysiword[0x21] = norword[0x406 + i * 5 + 2];
                    break;
                case 0x53724e6d: // SrNm
                    memcpy(&sysi[0x18], &norword[0x406 + i * 5 + 1], 16);
                    break;
                case 0x46774964: // FwId
                    if (nor[0x1018 + i * 0x14 + 7] == 0)
                    {
                        memcpy(&sysi[0x38], &norword[0x406 + i * 5 + 2], 3);
                        sysi[0x3b] = 2;
                        sysiword[0xf] = 0xa2700;
                    }
                    else memcpy(&sysi[0x38], &norword[0x406 + i * 5 + 2], 8);
                    memset(&sysi[0x40], 10, 0);
                    break;
                case 0x556e7443: // UntC
                    memcpy(&sysi[0xbc], &norword[0x406 + i * 5 + 1], 16);
                    break;
                case 0x52746341: // RtcA
                    if (norword[0x406 + i * 5 + 1] == 1)
                        sysiword[0x80] = norword[0x406 + i * 5 + 2];
                    break;
                case 0x42747279: // Btry
                    if (norword[0x406 + i * 5 + 1] == 1)
                        memcpy(&sysi[0x5c], &norword[0x406 + i * 5 + 2], 12);
                    break;
                case 0x5265676e: // Regn
                    if (nor[0x1018 + i * 0x14 + 4] == 1 && nor[0x1018 + i * 0x14 + 5] == 0)
                        memcpy(&sysi[0x92], &norword[0x406 + i * 5 + 2], 4);
                    break;
                case 0x4d6f6423: // Mod#
                    memcpy(&sysi[0x98], &norword[0x406 + i * 5 + 1], 16);
                    break;
                case 0x48774f31: // Hw01
                    memcpy(&sysi[0xa8], &norword[0x406 + i * 5 + 1], 16);
                    break;
                case 0x436f6e74: // Cont
                    if (nor[0x1018 + i * 0x14 + 4] == 0 && nor[0x1018 + i * 0x14 + 5] == 0)
                    {
                        sysi[0xb8] = nor[0x1018 + i * 0x14 + 6];
                        sysi[0xb9] = nor[0x1018 + i * 0x14 + 8];
                    }
                    break;
                case 0x426b4c74: // BkLt
                    if (nor[0x1018 + i * 0x14 + 4] == 0xaa && nor[0x1018 + i * 0x14 + 5] == 0x55)
                    {
                        sysi[0xba] = nor[0x1018 + i * 0x14 + 6];
                        sysi[0xbb] = nor[0x1018 + i * 0x14 + 8];
                    }
                    break;
                case 0x44726d56: // DrmV
                    memcpy(&sysi[0xce], &norword[0x406 + i * 5 + 1], 16);
                    break;
            }
        sysiword[2] = 0x646f5069;
        sysiword[3] = 0x36334e20;
        sysi[0x88] = 0x4e;
        sysi[0x89] = 0x41;
        sysiword[0x38] = 0x2000000;
        sysiword[0x39] = 0x8000000;
        sysiword[0x3a] = 0x2c000;
        sysiword[0x3b] = 0x22000000;
        sysiword[0x3c] = 0x100000;
        sysiword[0x3d] = 0x24000000;
        sysiword[0x4a] = 0x53797349;
        sysiword[0x4b] = 0x2202bdf0;
    }
}
