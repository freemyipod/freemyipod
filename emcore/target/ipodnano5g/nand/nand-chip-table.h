/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Michael Sparmann
 * Copyright (C) 2026 Viktor Krapivin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef NANO5G_NAND_CHIP_TABLE_H
#define NANO5G_NAND_CHIP_TABLE_H

#include "nand-target.h"

/* ==== Chip identification ==== */

/* The nano 5G drives the parts below. Geometry comes from this table, never
 * from the decoded READ ID bits: the page size and spare size encoding of
 * the ID differs from vendor to vendor.
 *
 * A part is matched by its first four READ ID bytes. The 8 KiB-page parts
 * have 16 ECC sectors per page; a part with two dies has twice the blocks
 * per chip enable and the second die starts at the next power-of-two
 * multiple of the per-die block count (see nand_block_row()). */
struct nand_part
{
    uint32_t id;                /* READ ID bytes 0..3 little endian (byte 0 = maker) */
    uint16_t blocks;            /* per chip enable (all dies) */
    uint16_t pages_per_block;
    uint16_t page_size;         /* data bytes */
    uint16_t spare;             /* spare bytes per page */
    uint8_t  vendor;            /* NAND_VENDOR_* class */
    uint8_t  dies;              /* dies per chip enable */
};

static const struct nand_part nand_parts[] =
{
    { 0x7294d7ec, 4152, 128, 8192, 436, NAND_VENDOR_SAMSUNG_8K  , 1 },  /* ec d7 94 72 */
    { 0x29d5d7ec, 8192, 128, 4096, 218, NAND_VENDOR_SAMSUNG     , 2 },  /* ec d7 d5 29 */
    { 0x2994d5ec, 4096, 128, 4096, 218, NAND_VENDOR_SAMSUNG     , 1 },  /* ec d5 94 29 */
    { 0xb614d5ec, 4096, 128, 4096, 128, NAND_VENDOR_SAMSUNG     , 1 },  /* ec d5 14 b6 */
    { 0xb655d7ec, 8192, 128, 4096, 128, NAND_VENDOR_SAMSUNG     , 2 },  /* ec d7 55 b6 */
    { 0xb614d5ad, 4096, 128, 4096, 128, NAND_VENDOR_HYNIX       , 1 },  /* ad d5 14 b6 */
    { 0xb655d7ad, 8192, 128, 4096, 128, NAND_VENDOR_HYNIX       , 2 },  /* ad d7 55 b6 */
    { 0x2594d7ad, 8192, 128, 4096, 224, NAND_VENDOR_HYNIX       , 1 },  /* ad d7 94 25 */
    { 0xba94d598, 4096, 128, 4096, 218, NAND_VENDOR_TOSHIBA     , 1 },  /* 98 d5 94 ba */
    { 0xba95d798, 8192, 128, 4096, 218, NAND_VENDOR_TOSHIBA     , 2 },  /* 98 d7 95 ba */
    { 0x3294d798, 4148, 128, 8192, 376, NAND_VENDOR_TOSHIBA     , 1 },  /* 98 d7 94 32 */
    { 0x3295de98, 8296, 128, 8192, 376, NAND_VENDOR_TOSHIBA     , 2 },  /* 98 de 95 32 */
    { 0x3294e798, 4100, 128, 8192, 448, NAND_VENDOR_TOSHIBA     , 1 },  /* 98 e7 94 32 */
    { 0x3e94d589, 4096, 128, 4096, 218, NAND_VENDOR_INTEL       , 1 },  /* 89 d5 94 3e */
    { 0x3ed5d789, 8192, 128, 4096, 218, NAND_VENDOR_INTEL       , 2 },  /* 89 d7 d5 3e */
    { 0x3e94d52c, 4096, 128, 4096, 218, NAND_VENDOR_MICRON      , 1 },  /* 2c d5 94 3e */
    { 0x3ed5d72c, 8192, 128, 4096, 218, NAND_VENDOR_MICRON      , 2 },  /* 2c d7 d5 3e */
    { 0x3e94d72c, 8192, 128, 4096, 218, NAND_VENDOR_MICRON_2    , 1 },  /* 2c d7 94 3e */
};

/* A population is one part on 2 or 4 chip enables (CE0..), the rest empty.
 * Only populations the nano 5G firmware drives are listed. Each row has the
 * bus timing of the population, eight values in ns that nand_calc_timing()
 * converts to controller clocks ([0..2] write cycle, [3..5] read cycle,
 * [6..7] read data valid and output disable times), and the layout word the
 * volume is formatted with (the low half says how the ways of a chip map to
 * blocks, see the FTL). */
struct nand_population
{
    uint8_t  groups;            /* groups of chip enables with this part (1) */
    uint8_t  chips;             /* populated chip enables per group: 2 or 4 */
    uint32_t id;                /* the part, nand_parts[].id */
    uint8_t  timing[8];         /* ns, see above */
    uint32_t layout;            /* low 16 bits: 1 = one way, 0x11/0x14 = two ways */
};

static const struct nand_population nand_populations[] =
{
    { 1, 2, 0x7294d7ec, { 30, 15, 10, 30, 15, 10, 25, 15 }, 0x00100014 },
    { 1, 4, 0x7294d7ec, { 30, 15, 10, 30, 15, 10, 25, 15 }, 0x00100014 },
    { 1, 4, 0x29d5d7ec, { 30, 15, 10, 30, 15, 10, 20, 15 }, 0x00100014 },
    { 1, 4, 0x2994d5ec, { 30, 15, 10, 30, 15, 10, 20, 15 }, 0x00100014 },
    { 1, 4, 0xb614d5ec, { 25, 12, 10, 25, 12, 10, 20, 15 }, 0x00100014 },
    { 1, 4, 0xb655d7ec, { 45, 25, 15, 50, 25, 15, 30, 15 }, 0x00010001 },
    { 1, 2, 0x2594d7ad, { 25, 12, 10, 25, 12, 10, 20, 15 }, 0x00100014 },
    { 1, 4, 0x2594d7ad, { 25, 12, 10, 25, 12, 10, 20, 15 }, 0x00100014 },
    { 1, 2, 0x3294d798, { 25, 12, 10, 25, 12, 10, 20, 25 }, 0x00150011 },
    { 1, 4, 0x3294d798, { 25, 12, 10, 25, 12, 10, 20, 25 }, 0x00150011 },
    { 1, 4, 0x3295de98, { 25, 12, 10, 25, 12, 10, 20, 25 }, 0x00150011 },
    { 1, 2, 0x3294e798, { 25, 12, 10, 25, 12, 10, 20, 25 }, 0x00150011 },
    { 1, 4, 0x3294e798, { 25, 12, 10, 25, 12, 10, 20, 25 }, 0x00150011 },
    { 1, 2, 0x3e94d72c, { 20, 10,  7, 20, 10,  7, 16, 15 }, 0x00120014 },
};

/* An absent chip enable reads all 0x00 or all 0xff */
static _Bool nand_ce_empty(uint32_t id)
{
    return id == 0 || id == 0xffffffff;
}

static const struct nand_part *nand_find_part(uint32_t id)
{
    unsigned int i;

    for (i = 0; i < (sizeof(nand_parts) / sizeof(nand_parts[0])); i++)
        if (nand_parts[i].id == id)
            return &nand_parts[i];
    return NULL;
}

/* Does what READ ID returned on CE0..7 match population p? Group g owns
 * CE 4g..4g+3: its first `chips` enables must report the part, the rest of
 * the group and every enable outside the groups must be empty. */
static _Bool nand_population_matches(const struct nand_population *p,
                                    const uint32_t *ids)
{
    unsigned int ce;

    for (ce = 0; ce < NAND_MAX_BANKS; ce++)
    {
        _Bool populated = ce / 4 < p->groups && ce % 4 < p->chips;

        if (populated ? ids[ce] != p->id : !nand_ce_empty(ids[ce]))
            return false;
    }
    return true;
}

static unsigned int nand_power_of_two(unsigned int value)
{
    unsigned int power = 1;
    while (power < value)
        power <<= 1;
    return power;
}

/* bl_NandReadOnly RVA 0x591c, matching at 0x522c: scan all eight CEs,
 * match the population and platform 0x16, then take geometry from the
 * chip row. The table above is the platform-0x16 join in firmware order.
 * All its fifth ID bytes are wildcards, so the four READ ID bytes suffice.
 * Keep this independent of controller I/O so every firmware row can be
 * checked against the original tables. */
static int nand_identify(const uint32_t ids[NAND_MAX_BANKS],
                         struct nand_geometry *g,
                         const struct nand_population **population)
{
    const struct nand_population *pop = NULL;
    const struct nand_part *part;
    unsigned int i, parity;

    memset(g, 0, sizeof(*g));
    memset(g->ce, 0xff, sizeof(g->ce));
    *population = NULL;
    for (i = 0; i < NAND_MAX_BANKS; i++)
        if (!nand_ce_empty(ids[i]))
        {
            g->ce[g->banks++] = i;
            g->id = ids[i];
        }
    for (i = 0; i < sizeof(nand_populations) / sizeof(nand_populations[0]); i++)
        if (nand_population_matches(&nand_populations[i], ids))
        {
            pop = &nand_populations[i];
            break;
        }
    if (!pop)
        return g->banks && nand_find_part(ids[g->ce[0]])
               ? NAND_ERR_UNSUPPORTED : NAND_ERR_NOCHIP;
    part = nand_find_part(pop->id);
    if (!part)
        return NAND_ERR_NOCHIP;

    g->blocks = part->blocks;
    g->pages_per_block = part->pages_per_block;
    g->page_size = part->page_size;
    g->spare_size = part->spare;
    g->vendor = part->vendor;
    g->layout = pop->layout;
    g->dies = part->dies;
    g->blocks_per_die = part->blocks / part->dies;
    if ((part->blocks & (part->blocks - 1)) == 0)
    {
        g->die_stride = g->blocks_per_die;
        g->block_span = g->blocks_aligned = part->blocks;
    }
    else
    {
        g->die_stride = nand_power_of_two(g->blocks_per_die);
        g->block_span = g->die_stride * (part->dies - 1) + g->blocks_per_die;
        g->blocks_aligned = part->blocks & ~0xfff;
    }
    g->page_stride = nand_power_of_two(part->pages_per_block);
    parity = (part->spare - 10) / (part->page_size / 512);
    if (parity < 13)
        return NAND_ERR_UNSUPPORTED;
    g->ecc_bits = parity >= 26 ? 16 : 8;
    g->refresh_threshold = parity >= 26 ? 12 : 6;
    *population = pop;
    return NAND_OK;
}

#endif
