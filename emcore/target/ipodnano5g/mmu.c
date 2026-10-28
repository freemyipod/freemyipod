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
#include "mmu.h"


/* ARM1176 range cache operations use 32-byte lines. Cleaning writes back
 * CPU data before DMA; invalidation discards lines after device writes. */

void clean_dcache_range(uint32_t addr, uint32_t size)
{
    uint32_t end = addr + size;
    addr &= ~31u;                       /* 32-byte line granularity */
    for (; addr < end; addr += 32u)
        __asm volatile("mcr p15, 0, %0, c7, c10, 1" :: "r"(addr));
    __asm volatile("mcr p15, 0, %0, c7, c10, 4" :: "r"(0));   /* DSB */
}

void invalidate_dcache_range(uint32_t addr, uint32_t size)
{
    uint32_t end = addr + size;
    addr &= ~31u;
    for (; addr < end; addr += 32u)
        __asm volatile("mcr p15, 0, %0, c7, c6, 1" :: "r"(addr));
    __asm volatile("mcr p15, 0, %0, c7, c10, 4" :: "r"(0));   /* DSB */
}


void clean_dcache()
{
    asm volatile(
        "MOV R0, #0                \n\t"
        "MCR p15, 0, R0,c7,c10,0   \n\t"
        "MCR p15, 0, R0,c7,c10,4   \n\t"
        "MOV PC, LR                \n\t"
    );
}

void invalidate_dcache()
{
    asm volatile(
        "MOV R0, #0                \n\t"
        "MCR p15, 0, R0,c7,c14,0   \n\t"
        "MCR p15, 0, R0,c7,c10,4   \n\t"
        "MOV PC, LR                \n\t"
    );
}

void invalidate_icache()
{
    asm volatile(
        "MOV R0, #0                \n\t"
        "MCR p15, 0, R0,c7,c5,0    \n\t"
        "MCR p15, 0, R0,c7,c5,4    \n\t"
        "MOV PC, LR                \n\t"
    );
}
