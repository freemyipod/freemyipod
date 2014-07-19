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
#include "bootflash.h"
#include "clickwheel.h"
#include "clockgates.h"
#include "s5l8702.h"


#define sysi ((uint8_t*)0x2203fdf8)
#define sysiword ((uint32_t*)0x2203fdf8)


void targetinit_late()
{
    int i;
    
    clickwheel_init();
    
    uint8_t* nor = (uint8_t*)memalign(0x10, 0x1000);
    uint32_t* norword = (uint32_t*)nor;
    if (!nor) return;
    bootflash_readraw(nor, 0, 0x1000);
    uint32_t scfg_size = norword[1];
    uint32_t scfg_entrycount = norword[5];
    if (norword[0] == 0x53436667 && scfg_size <= 0x1000
     && scfg_entrycount * 0x14 + 0x18 == scfg_size)
    {
        memset(sysi, 0, 0x128);
        sysiword[0] = 0x53797349;
        sysiword[1] = 4;
        sysiword[0x22] = 0x414e;
        sysiword[0x38] = 0x4000000;
        sysiword[0x39] = 0x8000000;
        sysiword[0x3a] = 0x40000;
        sysiword[0x3b] = 0x22000000;
        sysiword[0x3c] = 0x100000;
        sysiword[0x3d] = 0x24000000;
        sysiword[0x46] = 0x7672736e;
        sysiword[0x47] = 0x1308004;
        sysiword[0x48] = 0x53797349;
        sysiword[0x49] = 0x2203fdf8;
        for (i = 0; i < scfg_entrycount; i++)
            switch (norword[6 + i * 5])
            {
                case 0x53724e6d: // SrNm
                    memcpy(&sysi[0x18], &norword[6 + i * 5 + 1], 16);
                    break;
                case 0x46774964: // FwId
                    memcpy(&sysi[0x38], &norword[6 + i * 5 + 2], 8);
                    break;
                case 0x48775672: // HwVr
                    sysiword[0x21] = norword[6 + i * 5 + 2];
                    break;
                case 0x5265676e: // Regn
                    if (nor[24 + i * 0x14 + 4] == 1 && nor[24 + i * 0x14 + 5] == 0)
                        memcpy(&sysi[0x92], &norword[6 + i * 5 + 2], 4);
                    break;
                case 0x4d6f6423: // Mod#
                    memcpy(&sysi[0x98], &norword[6 + i * 5 + 1], 16);
                    break;
                case 0x436f6463: // Codc
                    sysiword[0x45] = norword[6 + i * 5 + 1];
                    break;
                case 0x53775672: // SwVr
                    memcpy(&sysi[0x108], &norword[6 + i * 5 + 1], 16);
                    break;
            }
		switch (sysiword[0x21])
		{
			case 0x130100:
				sysiword[0x47] = 0x1308004;
				break;
			case 0x130200:
				sysiword[0x47] = 0x1708004;
				break;
		}
    }
    free(nor);
}

void targetinit_execfirmware() ICODE_ATTR;
void targetinit_execfirmware()
{
    clockgate_enable(CLOCKGATE_I2C_0, true);
    while (IIC10(0));
    IICCON(0) = 0x184;
}

