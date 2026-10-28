/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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

/* VSVFL/FTL. VSVFL remaps bad blocks on each chip; the FTL maps
 * logical blocks and overlays newer pages from log blocks. Mount loads a
 * committed context or rebuilds the mapping from page metadata. */

#include "global.h"
#include "nand-target.h"
#include "ftl-target.h"
#include "libc/include/string.h"

/* ==== On-flash layout ==== */
/* The nano5g geometry is discovered at run time: two or four chip enables,
 * 4096- or 8192-byte physical pages, and one or two ways per chip. Logical
 * storage sectors remain 4096 bytes. */
#define FTL_PAGES_PER_BLOCK 128
#define FTL_MAX_BANKS       4
#define FTL_MAX_WAYS        2
#define FTL_MAX_PAGES_PER_VB (FTL_MAX_BANKS * FTL_MAX_WAYS * FTL_PAGES_PER_BLOCK)
#define FTL_MAX_BLOCKS      8448    /* physical blocks per chip (8296 at most) */
static unsigned int ftl_banks;      /* chip enables in use */
static unsigned int ftl_ways;       /* virtual banks per chip enable */
static unsigned int ftl_vbanks;     /* banks * ways */
static unsigned int ftl_ppvb;       /* FTL pages per virtual block */
static unsigned int ftl_page_bytes; /* data bytes of one FTL page */
#define FTL_BANKS           ftl_banks
#define FTL_VBANKS          ftl_vbanks
#define FTL_PAGES_PER_VB    ftl_ppvb
#define FTL_PAGE_BYTES      ftl_page_bytes
#define FTL_ROW(block, page) (nand_block_row(block) + (page))

/* Write support is checked when mounting the FTL. */

/* VSVFL context page (little endian; one 2048-byte context
 * per chip, at the start of a 4096-byte page whose meta is the VFL mark):
 *
 *   0x000 u32  usn_inc       newest context of the volume = largest value
 *   0x008 u32  ftl_type      2 = YAFTL
 *   0x026 u16  remap[820]    entry i: block replaced by reserve block
 *                            pool_start + i; 0xfff0 unused, 0xffff bad reserve
 *   0x68e u16  cxt_blocks[4] blocks this chip's context rotates through
 *   0x696 u16  usable        virtual blocks given to the FTL (+ 23 extra)
 *   0x698 u16  pool_start    first reserve block
 *   0x69a u16  ftl_ctrl[3]   the FTL's three control blocks (vb numbers)
 *   0x6da u32  vendor_type   address layout (unaligned)
 *   0x7f8 u32  sum1 = sum of the u32 words 0..0x7f4 + 0xaabbccdd
 *   0x7fc u32  sum2 = xor of the same words ^ 0xaabbccdd
 */
/* VFL context, one 2 KiB copy per chip */
#define VFL_CXT_SIZE        0x800  /* 2048 bytes */
#define VFL_CXT_USN         0x000   /* u32, increases with every update */
#define VFL_CXT_FTL_TYPE    0x008   /* u32, 2 = YAFTL (not handled) */
#define VFL_CXT_REMAP       0x026   /* u16[], block replaced by reserve i */
#define VFL_CXT_BLOCKS      0x68e   /* u16[4], blocks holding the context */
#define VFL_CXT_USABLE      0x696   /* u16, virtual blocks given to the FTL */
#define VFL_CXT_POOL_START  0x698   /* u16, first reserve block */
#define VFL_CXT_FTL_CTRL    0x69a   /* u16[3], the FTL's control blocks */
#define VFL_CXT_VENDOR      0x6da   /* u32 (unaligned), address layout */
#define VFL_CXT_VERSION     0x7f4   /* u32 format version (nano 5G) */
#define VFL_CXT_SUM1        0x7f8  /* u32 checksum 1 (sum) */
#define VFL_CXT_SUM2        0x7fc  /* u32 checksum 2 (xor) */
#define VFL_MAX_REMAP       ((VFL_CXT_BLOCKS - VFL_CXT_REMAP) / 2)  /* = 820 entries */
#define VFL_REMAP_UNUSED    0xfff0
#define VFL_REMAP_BAD       0xffff  /* the reserve block itself is bad */
#define VFL_SCAN_LAST       199     /* contexts live in blocks 1..199 */
#define VFL_CXT_COPIES      8       /* each update writes 8 copies */

/* FTL context, first 0x800 bytes of the newest control page
 * (meta type 'C'). Only the fields below are used here:
 *
 *   0x038 u32[18] vpns of the block map pages (u16 per logical block)
 *   0x080 u32[36] vpns of the erase counter pages (u16 per vb)
 *   0x110 u32[34] vpns of the log page offset pages (u16[17][512])
 *   0x1a4 18 x 20 log slots: +0 u32 USN, +4 u16 vb, +6 u16 lbn, rest unused
 *   0x320 u32[36] vpns of the read counter pages (u16 per vb)
 *   0x3d0 u32     vpn of the statistics page ('G')
 *   0x3d4 u32     must be 0xffffffff for the statistics page to be loaded
 */
/* FTL */
#define FTL_EXTRA_VBS       23      /* 3 control + 20 log/free blocks */
#define FTL_LOG_SLOTS       17      /* log blocks in use at most */
#define FTL_RESTORE_SLOTS   18      /* the rebuild uses one more as scratch */
#define FTL_FREE_SLOTS      20  /* log + free blocks always add up to 20 */
#define FTL_CXT_MAP_PAGES   0x038   /* u32[18] vpns of the block map */
#define FTL_CXT_EC_PAGES    0x080   /* u32[36] erase counter table */
#define FTL_CXT_OFF_PAGES   0x110   /* u32[34] log page offset tables */
#define FTL_CXT_LOGS        0x1a4   /* 18 x 20 bytes */
#define FTL_CXT_LOG_SIZE    0x14  /* FTLCxtLog entry size */
#define FTL_CXT_RC_PAGES    0x320   /* u32[36] read counter table */
#define FTL_CXT_STATS_PAGE  0x3d0  /* u32 vpn of the statistics page */
#define FTL_CXT_STATS_FLAG  0x3d4  /* u32, 0xffffffff = statistics page valid */

/* Metadata type byte (meta[9]). 0x44 'D' / 0x45 'E' map and
 * offset pages, 0x46 'F' erase counters, 0x47 'G' statistics, 0x49 'I' read
 * counters, 0x4f 'O' other control pages. */
/* Metadata type byte (meta[9]) */
#define META_LOG            0x40    /* page of a log block */
#define META_DATA           0x41    /* page of a data block */
#define META_CXT            0x43    /* FTL context */
#define META_CTRL_FIRST     0x43    /* control pages are 0x43..0x4f */
#define META_CTRL_LAST      0x4f
/* Block states during the rebuild */
/* 'B' and 'H' are RAM-only block states. */
#define VB_CTRL             0x42
#define VB_FREE             0x48

/* ==== In-RAM state ==== */

/* Per-block rebuild record: sequence number, logical block and block type. */
struct vb_state
{
    uint32_t usn;
    uint16_t lbn;
    uint8_t type;
};

/* Log block vbn holds newer pages of logical block lbn. */
struct ftl_log
{
    uint32_t usn;
    uint16_t vbn;               /* 0xffff = slot unused */
    uint16_t lbn;
};

/* VFL state */
static unsigned int vfl_blocks;  /* blocks per chip (nand geometry) */
static uint16_t vfl_pool_start;  /* first reserve block (newest context) */
static uint16_t vfl_usable;  /* virtual blocks of the FTL incl. 23 extra */
static uint16_t vfl_ftl_ctrl[3];  /* the FTL control blocks (vb numbers) */
static uint16_t vfl_remap[FTL_MAX_BANKS][VFL_MAX_REMAP];  /* per chip, from its own context */
static uint8_t vfl_bad[FTL_MAX_BANKS][FTL_MAX_BLOCKS / 8];  /* 1 bit per block, 1 = remapped (bad) */
static uint8_t vfl_cxt[FTL_MAX_BANKS][VFL_CXT_SIZE];  /* newest context of each chip */

/* FTL state */
static uint32_t ftl_vbs;        /* logical blocks */
static uint32_t ftl_pages;      /* logical pages */
static _Bool ftl_mounted, ftl_restored;
static uint16_t ftl_map[FTL_MAX_BLOCKS];  /* lbn -> vb (block map) */
static struct ftl_log ftl_logs[FTL_RESTORE_SLOTS];  /* 17 logs + 1 scratch slot */
/* per log: the log page holding logical page i of its block, 0xffff = none.
 * The first 17 rows are stored on flash as one table */
/* one row of FTL_PAGES_PER_VB entries (a run-time value) per slot */
static uint16_t ftl_offsets[FTL_RESTORE_SLOTS * FTL_MAX_PAGES_PER_VB];
#define FTL_OFFS_ROW(slot)  (&ftl_offsets[(slot) * FTL_PAGES_PER_VB])
#define FTL_OFFS(slot, off) (FTL_OFFS_ROW(slot)[off])
/* Recovered unwritten ranges must not expose stale blocks assigned to them. */
static _Bool ftl_blank[FTL_MAX_BLOCKS];
static int ftl_write_mount(void);
static int ftl_format_pending(void);
static _Bool ftl_writable;

static uint8_t ftl_cxt[VFL_CXT_SIZE];  /* FTL context (first 0x800 bytes) */
static struct vb_state ftl_vb[FTL_MAX_BLOCKS];     /* rebuild only */

static uint8_t ftl_buf[NAND_PAGE_SIZE] CACHEALIGN_ATTR;  /* page buffer for metadata scans */
static uint8_t ftl_meta[NAND_MAX_PAGES_PER_READ * NAND_META_SIZE]  /* meta of up to 16 pages */
    __attribute__((aligned(4)));

/* Little-endian loads from byte buffers (on-flash structures are LE and
 * not always aligned, e.g. VFL_CXT_VENDOR) */
static uint16_t get16(const uint8_t *p)
{
    return p[0] | (p[1] << 8);
}

static uint32_t get32(const uint8_t *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put16(uint8_t *p, uint16_t value)
{
    p[0] = value;
    p[1] = value >> 8;
}

static void put32(uint8_t *p, uint32_t value)
{
    put16(p, value);
    put16(p + 2, value >> 16);
}

/*
 * VFL
 */

/* Read a physical context page into ftl_buf/ftl_meta. Metadata bytes 8/9
 * must be 0x00/0x80. */
/* One copy of a context page: any of the 8 copies at page..page+7 that
 * reads cleanly and carries the context mark */

static _Bool vfl_read_cxt_page(unsigned int bank, unsigned int block,
                              unsigned int page)
{
    unsigned int i;

    for (i = 0; i < VFL_CXT_COPIES; i++)
        if (nand_read_page(bank, FTL_ROW(block, page + i),
                           ftl_buf, ftl_meta) == NAND_OK
            && ftl_meta[8] == 0x00 && ftl_meta[9] == 0x80
            /* the context carries a format version, 2 at most is known */
            && get32(ftl_buf + VFL_CXT_VERSION) <= 2
            )
            return true;
    return false;
}

/* Find a named block in the top tenth of the chip and copy len payload bytes. */
/* Scan blocks downward, stopping each page scan after three erased pages.
 * Header: name at +0x00, BBT entries at +0x14, length at +0x34, payload at
 * +0x38. Only the first matching page is needed. */
static _Bool vfl_find_special(unsigned int bank, const char *name,
                             uint8_t *payload, unsigned int len)
{
    unsigned int n, lo = vfl_blocks - vfl_blocks / 10;

    for (n = vfl_blocks; n >= lo; n--)
    {
        unsigned int page, blank = 0;
        for (page = 0; page < FTL_PAGES_PER_BLOCK && blank < 3; page++)
        {
            int rc = nand_read_page(bank, FTL_ROW(n - 1, page),
                                    ftl_buf, NULL);
            if (rc == NAND_ERASED)
                blank++;
            else if (rc == NAND_OK && !memcmp(ftl_buf, name, 16))
            {
                if (payload)
                    memcpy(payload, ftl_buf + 0x38, len);
                return true;
            }
        }
    }
    return false;
}

/* Accept signature 'C', FTL type '0'/'1', VFL type '1', format version < 5. */
/* The NANDDRIVERSIGN payload's first u32 (LE): byte 3 'C', byte 2 FTL type
 * '0' / '1', byte 1 VFL type ('1' = VSVFL; '0', the classic VFL, is
 * refused, as WMR_SelectVFL does with the signature style the bootloader
 * uses), byte 0 not checked; second u32 = format version. */
static _Bool vfl_check_signature(void)
{
    static const char name[16] = "NANDDRIVERSIGN";
    uint8_t sig[8];
    uint32_t w;

    if (!vfl_find_special(0, name, sig, sizeof(sig)))
        return false;
    w = get32(sig);
    if ((w >> 24) != 'C' || ((w >> 8) & 0xff) != '1')
        return false;
    if (((w >> 16) & 0xff) != '0' && ((w >> 16) & 0xff) != '1')
        return false;
    if (((w >> 16) & 0xff) == '1' && get32(sig + 4) >= 5)
        return false;
    return true;
}

/* Load the newest context of one chip into vfl_cxt[bank] */
/* Scan blocks 1..199 for context blocks. best == 4 means none found. */
static _Bool vfl_load_cxt(unsigned int bank)
{
    uint8_t *cxt = vfl_cxt[bank];
    unsigned int block, i, best = 4, last = 0, page;
    uint32_t best_usn = 0xffffffff, sum = 0, xor = 0;

    /* Any copy names the (up to) four blocks the context rotates through */
    for (block = 1; block <= VFL_SCAN_LAST; block++)
        if (vfl_read_cxt_page(bank, block, 0))
            break;
    if (block > VFL_SCAN_LAST)
        return false;
    memcpy(cxt + VFL_CXT_BLOCKS, ftl_buf + VFL_CXT_BLOCKS, 8);

    /* The block in use is the one whose first page has the lowest nonzero
     * sequence number in its metadata (it counts down) */
    for (i = 0; i < 4; i++)
    {
        uint16_t b = get16(cxt + VFL_CXT_BLOCKS + 2 * i);
        uint32_t usn;
        if (b == 0xffff || !vfl_read_cxt_page(bank, b, 0))  /* 0xffff = slot unused */
            continue;
        usn = get32(ftl_meta);
        if (usn != 0 && usn <= best_usn)
        {
            best_usn = usn;
            best = i;
        }
    }
    if (best == 4)
        return false;
    block = get16(cxt + VFL_CXT_BLOCKS + 2 * best);

    /* Contexts are appended in groups of 8 pages: take the last group */
    for (page = 8; page < FTL_PAGES_PER_BLOCK
                   && vfl_read_cxt_page(bank, block, page); page += 8)
        last = page;
    if (!vfl_read_cxt_page(bank, block, last))
        return false;
    memcpy(cxt, ftl_buf, VFL_CXT_SIZE);

    /* the nano 5G firmware never verifies the checksums */
    (void)sum;
    (void)xor;
    return true;
}

/* Physical block of block vbn of virtual bank vbank (bank + banks * way).
 * With two ways the even and the odd blocks of a chip belong to different
 * ways. */
static unsigned int vfl_c2p(unsigned int vbank, unsigned int vbn)
{
    return ftl_ways == 2 ? 2 * vbn + vbank / ftl_banks : vbn;
}

/* Physical block of the reserve block that replaces a bad one: reserve
 * entry i of a chip belongs to way i / per, where per is the number of
 * reserve blocks of one way */
static unsigned int vfl_reserve_block(unsigned int bank, unsigned int i)
{
    unsigned int per = vfl_blocks / ftl_ways - vfl_pool_start;

    return vfl_c2p((i / per) * ftl_banks + bank, vfl_pool_start + i % per);
}
#define VFL_WAYS            ftl_ways

/* Load per-chip contexts and bad-block maps. Volume-wide fields come from
 * the context with the largest usn_inc. Reject unsupported address layouts
 * and values exceeding the static tables. */
static int vfl_open(void)
{
    static const char bbt_name[16] = "DEVICEINFOBBT";
    const uint8_t *newest = NULL;
    uint32_t newest_usn = 0, vendor;
    unsigned int bank, i, entries;

    /* Require a volume signature and factory bad-block table on every chip.
 * The remap tables in the VFL contexts supply the current bad-block list. */
    if (!vfl_check_signature())
        return NAND_ERR_FTL;
    for (bank = 0; bank < FTL_BANKS; bank++)
    {
        if (!vfl_find_special(bank, bbt_name, NULL, 0) || !vfl_load_cxt(bank))
            return NAND_ERR_FTL;
        if (get32(vfl_cxt[bank] + VFL_CXT_USN) >= newest_usn)
        {
            newest_usn = get32(vfl_cxt[bank] + VFL_CXT_USN);
            newest = vfl_cxt[bank];
        }
    }

    /* Use volume-wide values from the newest context. */
    vfl_usable = get16(newest + VFL_CXT_USABLE);
    vfl_pool_start = get16(newest + VFL_CXT_POOL_START);
    for (i = 0; i < 3; i++)
        vfl_ftl_ctrl[i] = get16(newest + VFL_CXT_FTL_CTRL + 2 * i);
    uint32_t type = get32(newest + VFL_CXT_FTL_TYPE);
    /* Restore-OS open (0x08042484) distributes the newest volume fields,
     * including the pending record, but takes address layout from CE0. */
    for (bank = 0; bank < FTL_BANKS; bank++)
    {
        uint8_t *c = vfl_cxt[bank];
        put32(c + VFL_CXT_FTL_TYPE, type);
        put16(c + VFL_CXT_USABLE, vfl_usable);
        put16(c + VFL_CXT_POOL_START, vfl_pool_start);
        memcpy(c + VFL_CXT_FTL_CTRL, newest + VFL_CXT_FTL_CTRL, 6);
        memcpy(c + 0x6ca, newest + 0x6ca, 16);
    }
    if (nand_write_supported() && newest[0x6d9])
        type = get32(newest + 0x6ca);
    if (type == 2)
        return NAND_ERR_UNSUPPORTED;
    /* Address layout: 0 = the chip's default, 0x00010001 for every nano 4G
     * chip. Only one way per chip (identity block mapping) is handled. */
    vendor = get32(newest + VFL_CXT_VENDOR);
    vendor = get32(vfl_cxt[0] + VFL_CXT_VENDOR);
    /* Low half of the word = how ways map to blocks: 1 = one way per chip;
     * 0x11 and 0x14 = two ways, even and odd blocks of each chip. */
    if (vendor == 0)
        vendor = nand_get_geometry()->layout;
    switch (vendor & 0xffff)
    {
        case 0x01:
            ftl_ways = 1;
            break;
        case 0x11:
        case 0x14:
            ftl_ways = 2;
            break;
        default:
            return NAND_ERR_UNSUPPORTED;
    }
    ftl_vbanks = ftl_banks * ftl_ways;
    ftl_ppvb = ftl_vbanks * FTL_PAGES_PER_BLOCK;
    if (vfl_pool_start * VFL_WAYS > vfl_blocks
        || vfl_blocks - vfl_pool_start * VFL_WAYS > VFL_MAX_REMAP
        || vfl_usable <= FTL_EXTRA_VBS
        || vfl_usable > vfl_pool_start)
        return NAND_ERR_FTL;

    /* Bad-block bitmap: a block is bad if a reserve block replaces it, and
     * a bad reserve block is bad too */
    entries = vfl_blocks - vfl_pool_start * VFL_WAYS;
    memset(vfl_bad, 0, sizeof(vfl_bad));
    for (bank = 0; bank < FTL_BANKS; bank++)
        for (i = 0; i < entries; i++)
        {
            uint16_t e = get16(vfl_cxt[bank] + VFL_CXT_REMAP + 2 * i);
            vfl_remap[bank][i] = e;
            if (e == VFL_REMAP_UNUSED)
                continue;
            if (e == VFL_REMAP_BAD)  /* the reserve block itself is bad */
                e = vfl_reserve_block(bank, i);
            if (e < vfl_blocks)
                vfl_bad[bank][e >> 3] |= 1 << (e & 7);
        }
    return 0;
}

/* Physical block of a virtual block on one chip, 0xffff if lost */
/* A bad block maps to pool_start + i where remap[i] equals its block number. */
static unsigned int vfl_remap_block(unsigned int bank, unsigned int block)
{
    unsigned int i;

    if (block >= vfl_blocks)
        return 0xffff;
    if (!(vfl_bad[bank][block >> 3] & (1 << (block & 7))))
        return block;
    for (i = 0; i < vfl_blocks - vfl_pool_start * VFL_WAYS; i++)
        if (vfl_remap[bank][i] == block)
            return vfl_reserve_block(bank, i);
    return 0xffff;
}

/* Virtual pages stripe across the virtual banks (four chips on the nano 4G).
 * Fail if a bad block has no reserve. */
static _Bool vfl_locate(uint32_t vpn, uint8_t *bank, uint32_t *page)
{
    unsigned int block, vbank = vpn % ftl_vbanks;

    *bank = vbank % ftl_banks;
    block = vfl_remap_block(*bank, vfl_c2p(vbank, vpn / ftl_ppvb));
    if (block == 0xffff)
        return false;
    *page = FTL_ROW(block, (vpn % ftl_ppvb) / ftl_vbanks);
    return true;
}

/* Read one virtual page: NAND_OK, NAND_ERASED or NAND_ERR_* */
/* Read 4096 data bytes and 12 metadata bytes; lost blocks return NAND_ERR_ECC. */
static int vfl_read(uint32_t vpn, void *data, uint8_t *meta)
{
    uint8_t bank;
    uint32_t page;

    if (!vfl_locate(vpn, &bank, &page))
        return NAND_ERR_ECC;
    return nand_read_page(bank, page, data, meta);
}

#include "ftl-vfl-write.h"

/*
 * FTL: mount from the stored context
 */

/* Read a table stored in the pages listed at cxt + list; every page must
 * read cleanly. dst may be NULL to only check readability. */
/* list = offset in ftl_cxt of a u32 vpn list, npages = pages to read, size
 * = bytes of the table (the last page is copied partially). */

static _Bool ftl_read_table(unsigned int list, unsigned int npages, void *dst,
                           uint32_t size)
{
    unsigned int i;

    for (i = 0; i < npages; i++)
    {
        uint32_t n = MIN(size - i * FTL_PAGE_BYTES, (uint32_t)FTL_PAGE_BYTES);
        if (vfl_read(get32(ftl_cxt + list + 4 * i), ftl_buf, ftl_meta)
            != NAND_OK)
            return false;
        if (dst)
            memcpy((uint8_t *)dst + i * FTL_PAGE_BYTES, ftl_buf, n);
    }
    return true;
}

/* Load the stored FTL context. Return false if a rebuild is required. */
static _Bool ftl_load(void)
{
    unsigned int i, best = 0xffff, page, total = ftl_vbs + FTL_EXTRA_VBS;
    uint32_t best_usn = 0xffffffff;
    _Bool ok = false;
    int rc;

    /* The current control block: lowest sequence number on page 0 */
    for (i = 0; i < 3; i++)
    {
        rc = vfl_read(vfl_ftl_ctrl[i] * FTL_PAGES_PER_VB, ftl_buf, ftl_meta);
        if (rc == NAND_ERR_TIMEOUT || rc == NAND_ERR_NOCHIP)
            return false;
        if (rc == NAND_OK && ftl_meta[9] >= META_CTRL_FIRST
            && ftl_meta[9] <= META_CTRL_LAST
            && (best == 0xffff || get32(ftl_meta) < best_usn))
        {
            best = vfl_ftl_ctrl[i];
            best_usn = get32(ftl_meta);
        }
    }
    if (best == 0xffff)
        return false;

    /* Its last written page must be an FTL context; anything else (an
     * "invalid" mark, a table page) means the context is stale */
    for (page = FTL_PAGES_PER_VB; page-- > 0; )
    {
        rc = vfl_read(best * FTL_PAGES_PER_VB + page, ftl_buf, ftl_meta);
        if (rc == NAND_OK)
        {
            if (ftl_meta[9] == META_CXT)
            {
                memcpy(ftl_cxt, ftl_buf, sizeof(ftl_cxt));
                ok = true;
            }
            break;
        }
        if (rc != NAND_ERASED)
            break;
    }
    if (!ok)
        return false;

    /* Require readable block-map, log-offset, counter and statistics pages. */
    if (!ftl_read_table(FTL_CXT_MAP_PAGES,
                        (ftl_vbs * 2 + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES,
                        ftl_map, ftl_vbs * 2))
        return false;
    if (!ftl_read_table(FTL_CXT_OFF_PAGES,
                        (FTL_LOG_SLOTS * FTL_PAGES_PER_VB * 2 + FTL_PAGE_BYTES - 1)
                            / FTL_PAGE_BYTES,
                        ftl_offsets, FTL_LOG_SLOTS * FTL_PAGES_PER_VB * 2))
        return false;
    if (!ftl_read_table(FTL_CXT_EC_PAGES,
                        (total * 2 + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES,
                        NULL, total * 2)
        || !ftl_read_table(FTL_CXT_RC_PAGES,
                           (total * 2 + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES,
                           NULL, total * 2))
        return false;
    /* Check the statistics page only when its virtual block is below ftl_vbs. */
    if (get32(ftl_cxt + FTL_CXT_STATS_FLAG) == 0xffffffff)
    {
        uint32_t vpn = get32(ftl_cxt + FTL_CXT_STATS_PAGE);
        uint16_t vb = (uint16_t)(vpn / FTL_PAGES_PER_VB);
        if ((vb == 0 || vb < ftl_vbs)
            && vfl_read(vpn, ftl_buf, ftl_meta) != NAND_OK)
            return false;
    }

    /* Load all 18 log slots; slot 17 is scratch, normally vbn 0xffff. */
    for (i = 0; i < FTL_RESTORE_SLOTS; i++)
    {
        const uint8_t *l = ftl_cxt + FTL_CXT_LOGS + i * FTL_CXT_LOG_SIZE;
        ftl_logs[i].usn = get32(l);
        ftl_logs[i].vbn = get16(l + 4);
        ftl_logs[i].lbn = get16(l + 6);
    }
    return true;
}

static void ftl_reset_log(unsigned int slot)
{
    memset(FTL_OFFS_ROW(slot), 0xff, FTL_PAGES_PER_VB * sizeof(uint16_t));
}

/* Index log offsets by lpn % 512. Skip erased and unreadable pages. */
/* Fill a log's offset table from the metadata of its pages; a logical page
 * written twice is found in the later copy */
static void ftl_scan_log(unsigned int slot)
{
    unsigned int p;
    uint32_t base = ftl_logs[slot].vbn * FTL_PAGES_PER_VB;

    for (p = 0; p < FTL_PAGES_PER_VB; p++)
        if (vfl_read(base + p, ftl_buf, ftl_meta) == NAND_OK)
            FTL_OFFS(slot, get32(ftl_meta) % FTL_PAGES_PER_VB) = p;
}

/* Erased or unreadable pages do not disqualify a sequential log. */
/* A log block whose pages are all in place can serve as data block */
static _Bool ftl_log_is_sequential(unsigned int vb)
{
    unsigned int p;

    for (p = 0; p < FTL_PAGES_PER_VB; p++)
        if (vfl_read(vb * FTL_PAGES_PER_VB + p, ftl_buf, ftl_meta) == NAND_OK
            && get32(ftl_meta) % FTL_PAGES_PER_VB != p)
            return false;
    return true;
}

/* Search active slots 0..16, excluding scratch. */
static struct ftl_log *ftl_find_log(unsigned int lbn)
{
    unsigned int i;

    for (i = 0; i < FTL_LOG_SLOTS; i++)
        if (ftl_logs[i].vbn != 0xffff && ftl_logs[i].lbn == lbn)
            return &ftl_logs[i];
    return NULL;
}

/* The control-page types _FTLRestore lists: 'C' 'D' 'E' 'F' 'G' 'I' 'O' */
static _Bool ftl_is_ctrl_type(uint8_t t)
{
    return t == 0x43 || t == 0x44 || t == 0x45 || t == 0x46 || t == 0x47
        || t == 0x49 || t == 0x4f;
}

/* Two log blocks claim the same logical block and one of them was being
 * written (it has the volume's highest sequence number): keep the newer one
 * only if it covers every page the older one has. Returns true if the
 * block being placed (vb) wins. */
/* Resolve competing logs after power loss. Scan both through scratch slot
 * 17, keep the winner and mark the loser free. Offset tables are rebuilt later. */
static _Bool ftl_log_conflict(unsigned int slot, unsigned int vb,
                             uint32_t newest_usn)
{
    const unsigned int scratch = FTL_RESTORE_SLOTS - 1;
    struct vb_state *e = &ftl_vb[vb];
    const _Bool vb_is_newer = (e->usn == newest_usn);
    const uint16_t *newer, *older;
    _Bool drop_vb;
    unsigned int j;

    ftl_logs[scratch].usn = e->usn;
    ftl_logs[scratch].lbn = e->lbn;
    ftl_logs[scratch].vbn = vb;
    ftl_reset_log(scratch);
    ftl_reset_log(slot);
    ftl_scan_log(scratch);
    ftl_scan_log(slot);
    newer = FTL_OFFS_ROW(vb_is_newer ? scratch : slot);
    older = FTL_OFFS_ROW(vb_is_newer ? slot : scratch);
    for (j = 0; j < FTL_PAGES_PER_VB; j++)
        if (older[j] != 0xffff && newer[j] == 0xffff)
            break;
    /* drop the older block if the newer one covers it, else the newer one;
     * the choice is made by sequence number, so on a tie vb loses */
    drop_vb = (j == FTL_PAGES_PER_VB) ? !vb_is_newer : vb_is_newer;
    if (!drop_vb && ftl_logs[slot].usn == e->usn)
        drop_vb = true;

    ftl_logs[scratch].vbn = 0xffff;
    ftl_logs[scratch].lbn = 0xffff;
    ftl_reset_log(scratch);
    ftl_reset_log(slot);
    if (drop_vb)
    {
        e->type = VB_FREE;
        return false;
    }
    ftl_vb[ftl_logs[slot].vbn].type = VB_FREE;
    return true;
}

/* Rebuild maps from block metadata in RAM. newest_usn identifies the block
 * being written at power loss. Reads 1..2 pages per block and every log page. */
static _Bool ftl_restore(void)
{
    const unsigned int nvb = ftl_vbs, total = ftl_vbs + FTL_EXTRA_VBS;
    uint16_t free_vbs[FTL_FREE_SLOTS];
    unsigned int vb, lbn, s, used, nfree, ndata, nlog, nfreevb;
    uint32_t newest_usn = 0, lpn;
    int rc;

    memset(ftl_vb, 0xff, total * sizeof(ftl_vb[0]));
    memset(ftl_map, 0xff, sizeof(ftl_map));
    for (s = 0; s < FTL_RESTORE_SLOTS; s++)
    {
        ftl_logs[s].vbn = 0xffff;
        ftl_logs[s].lbn = 0xffff;
        ftl_logs[s].usn = 0xffffffff;
        ftl_reset_log(s);
    }

    /* 1. the control blocks are never data */
    for (s = 0; s < 3; s++)
    {
        unsigned int cb = vfl_ftl_ctrl[s];
        if (cb >= total)  /* Rockbox bound: index into ftl_vb */
            return false;
        if (vfl_read(cb * FTL_PAGES_PER_VB, ftl_buf, ftl_meta) == NAND_OK
            && ftl_meta[9] >= META_CTRL_FIRST && ftl_meta[9] <= META_CTRL_LAST)
            ftl_vb[cb].usn = get32(ftl_meta);
        ftl_vb[cb].type = VB_CTRL;
    }

    /* 2. classify every block by its last page, or else its first page.
     * A full data block names its logical block; of two, the newer wins. */
    for (vb = 0; vb < total; vb++)
    {
        struct vb_state *e = &ftl_vb[vb];
        if (e->type == VB_CTRL)
            continue;
        e->type = VB_FREE;
        memset(ftl_meta, 0xff, NAND_META_SIZE);
        if (vfl_read(vb * FTL_PAGES_PER_VB + FTL_PAGES_PER_VB - 1,
                     ftl_buf, ftl_meta) == NAND_OK)
        {
            uint8_t t = ftl_meta[9];
            if (t == META_LOG || t == META_DATA)
            {
                lpn = get32(ftl_meta);
                lbn = (lpn / FTL_PAGES_PER_VB) & 0xffff;
                e->usn = get32(ftl_meta + 4);
                if (lbn != 0 && lbn >= nvb)
                    continue;   /* stays free */
                e->type = t;
                e->lbn = lbn;
                if (t == META_DATA)
                {
                    if (ftl_map[lbn] == 0xffff)
                        ftl_map[lbn] = vb;
                    else if (ftl_vb[ftl_map[lbn]].usn < e->usn)
                    {
                        ftl_vb[ftl_map[lbn]].type = VB_FREE;
                        ftl_map[lbn] = vb;
                    }
                    else
                        e->type = VB_FREE;
                }
                newest_usn = MAX(newest_usn, e->usn);
            }
            else if (ftl_is_ctrl_type(t))
                e->usn = get32(ftl_meta);   /* stale control block */
        }
        else
        {
            memset(ftl_meta, 0xff, NAND_META_SIZE);
            rc = vfl_read(vb * FTL_PAGES_PER_VB, ftl_buf, ftl_meta);
            if (rc != NAND_OK)
                continue;
            if (ftl_meta[9] == META_LOG)  /* only log blocks may be partly written */
            {
                /* a log block still being filled */
                lpn = get32(ftl_meta);
                lbn = (lpn / FTL_PAGES_PER_VB) & 0xffff;
                e->usn = get32(ftl_meta + 4);
                if (lbn == 0 || lbn < nvb)
                {
                    e->type = META_LOG;
                    e->lbn = lbn;
                    newest_usn = MAX(newest_usn, e->usn);
                }
            }
            else if (ftl_is_ctrl_type(ftl_meta[9]))
                e->usn = get32(ftl_meta);
        }
    }

    /* 3. log blocks into log slots */
    for (vb = 0; vb < total; vb++)
    {
        struct vb_state *e = &ftl_vb[vb];
        uint32_t data_usn = 0;
        if (e->type != META_LOG)
            continue;
        lbn = e->lbn;
        if (ftl_map[lbn] == 0xffff)
        {
            if (ftl_log_is_sequential(vb))
            {
                /* complete and in order: it is the data block */
                ftl_map[lbn] = vb;
                e->type = META_DATA;
                continue;
            }
        }
        else
            data_usn = ftl_vb[ftl_map[lbn]].usn;
        if (e->usn < data_usn)
        {
            e->type = VB_FREE;      /* older than its data block */
            continue;
        }
        for (s = 0; s < FTL_RESTORE_SLOTS; s++)
        {
            struct ftl_log *l = &ftl_logs[s];
            _Bool take = true;
            if (l->lbn != 0xffff)
            {
                if (l->lbn != lbn)
                    continue;
                if (e->usn != newest_usn && l->usn != newest_usn)
                {
                    /* two complete logs: the newer one wins */
                    if (ftl_vb[l->vbn].usn < e->usn)
                        ftl_vb[l->vbn].type = VB_FREE;
                    else
                    {
                        e->type = VB_FREE;
                        take = false;
                    }
                }
                else
                    take = ftl_log_conflict(s, vb, newest_usn);
            }
            if (take)
            {
                l->usn = e->usn;
                l->lbn = lbn;
                l->vbn = vb;
            }
            break;
        }
    }
    if (ftl_logs[FTL_RESTORE_SLOTS - 1].vbn != 0xffff)
    {
        /* An 18th log replaces the slot with the newest sequence number. */
        for (s = 0; s < FTL_RESTORE_SLOTS; s++)
            if (ftl_logs[s].usn == newest_usn)
                break;
        if (s == FTL_RESTORE_SLOTS)
            return false;
        ftl_vb[ftl_logs[s].vbn].type = VB_FREE;
        ftl_logs[s] = ftl_logs[FTL_RESTORE_SLOTS - 1];
    }

    /* 4. logical blocks nobody claims get free blocks (they read as 0xff) */
    for (lbn = 0, vb = 0; lbn < nvb && vb < total; lbn++)
    {
        if (ftl_map[lbn] != 0xffff)
            continue;
        for (; vb < total; vb++)
            if (ftl_vb[vb].type == VB_FREE)
            {
                ftl_map[lbn] = vb;
                ftl_blank[lbn] = true;
                ftl_vb[vb].lbn = lbn;
                ftl_vb[vb].type = META_DATA;
                vb++;
                break;
            }
    }

    /* 5. the remaining free blocks fill the free list after the logs */
    for (used = 0; used < FTL_LOG_SLOTS && ftl_logs[used].lbn != 0xffff; used++);
    nfree = 0;
    for (s = used, vb = 0; s < FTL_FREE_SLOTS && vb < total; s++)
        for (; vb < total; vb++)
            if (ftl_vb[vb].type == VB_FREE)
            {
                free_vbs[nfree++] = vb++;
                break;
            }
    if (used + nfree != FTL_FREE_SLOTS)
        return false;

    for (lbn = 0; lbn < nvb; lbn++)
        if (ftl_map[lbn] >= total || ftl_vb[ftl_map[lbn]].lbn != lbn)
            return false;
    ndata = nlog = nfreevb = 0;
    for (vb = 0; vb < total; vb++)
    {
        struct vb_state *e = &ftl_vb[vb];
        struct ftl_log *l;
        switch (e->type)
        {
            case META_LOG:
                nlog++;
                l = ftl_find_log(e->lbn);
                if (!l || l->vbn != vb)
                    return false;
                break;
            case META_DATA:
                ndata++;
                if ((e->lbn != 0 && e->lbn >= nvb) || ftl_map[e->lbn] != vb)
                    return false;
                break;
            case VB_CTRL:
                break;
            case VB_FREE:
                nfreevb++;
                for (s = 0; s < nfree && free_vbs[s] != vb; s++);
                if (s == nfree)
                    return false;
                break;
            default:
                return false;
        }
    }
    if (ndata != nvb || nfreevb != nfree || nlog != FTL_FREE_SLOTS - nfree)
        return false;

    /* Rebuild the winning log offset tables. */
    /* 7. offset tables of the logs in use */
    for (s = 0; s < FTL_LOG_SLOTS; s++)
    {
        ftl_reset_log(s);
        if (ftl_logs[s].vbn != 0xffff)
            ftl_scan_log(s);
    }
    return true;
}

/* ==== Public API ==== */

/* Mount once. Reserve 23 virtual blocks: three control and 20 log/free. */
int ftl_init(void)
{
    const struct nand_geometry *g = nand_get_geometry();
    int rc;

    if (ftl_mounted)
        return 0;
    if (!g || (g->banks != 2 && g->banks != 4)
        || g->pages_per_block != FTL_PAGES_PER_BLOCK
        || (g->page_size != 4096 && g->page_size != 8192)
        || g->blocks > FTL_MAX_BLOCKS)
        return NAND_ERR_UNSUPPORTED;
    ftl_banks = g->banks;
    ftl_page_bytes = g->page_size;
    vfl_blocks = g->blocks;

    rc = vfl_open();
    if (rc)
        return rc;

    ftl_writable = false;
    if (nand_write_supported())
    {
        rc = vfl_write_init();
        if (!rc)
            rc = vfl_apply_pending_geometry();
        if (rc)
            return rc;
    }

    ftl_vbs = vfl_usable - FTL_EXTRA_VBS;
    ftl_pages = ftl_vbs * FTL_PAGES_PER_VB;
    ftl_restored = false;
    if (nand_write_supported() && vfl_cxt[0][0x6d9])
        return ftl_format_pending();
    memset(ftl_blank, 0, sizeof(ftl_blank));
    if (!ftl_load())
    {
        ftl_restored = true;
        if (!ftl_restore())
            return NAND_ERR_FTL;
    }
    ftl_pages = ftl_vbs * FTL_PAGES_PER_VB;
    if (nand_write_supported())
    {
        rc = ftl_write_mount();
        if (rc)
            return rc;
        ftl_writable = true;
    }
    ftl_mounted = true;
    return 0;
}

/* Logical pages (4096 bytes each) = ftl_vbs * 512 */
/* The FTL page is the NAND page, 4096 or 8192 bytes; the sectors the
 * caller sees are always 4096 bytes. */
#define FTL_SECTORS_PER_PAGE (ftl_page_bytes / 4096)
uint32_t ftl_num_sectors(void)
{
    return ftl_mounted ? ftl_pages * FTL_SECTORS_PER_PAGE : 0;
}

_Bool ftl_was_restored(void)
{
    return ftl_restored;
}

/* ==== Logical read ==== */

/* The log's offset table (0xffff = page not in the log) wins over the
 * block map; data blocks hold logical page off at page off. */
static uint32_t ftl_vpn(uint32_t lpn)
{
    unsigned int lbn = lpn / FTL_PAGES_PER_VB, off = lpn % FTL_PAGES_PER_VB;
    struct ftl_log *log = ftl_find_log(lbn);
    unsigned int slot = log ? (unsigned int)(log - ftl_logs) : 0;

    if (log && FTL_OFFS(slot, off) != 0xffff)
        return log->vbn * FTL_PAGES_PER_VB + FTL_OFFS(slot, off);
    return ftl_map[lbn] * FTL_PAGES_PER_VB + off;
}

/* The FTL marks pages whose data it knows to be bad */

static _Bool ftl_meta_bad(const uint8_t *meta)
{
    return meta[8] != 0xff || meta[10] != 0xff;
}

static int ftl_read_current(uint32_t lpn, void *data)
{
    unsigned int lbn = lpn / FTL_PAGES_PER_VB;
    struct ftl_log *log = ftl_find_log(lbn);
    _Bool logged = log && FTL_OFFS(log - ftl_logs, lpn % FTL_PAGES_PER_VB)
                         != 0xffff;
    int rc;
    if (!logged && ftl_blank[lbn])
    {
        memset(data, 0xff, FTL_PAGE_BYTES);
        return 0;
    }
    rc = vfl_read(ftl_vpn(lpn), data, ftl_meta);
    if (rc == NAND_ERASED)
    {
        memset(data, 0xff, FTL_PAGE_BYTES);
        return 0;
    }
    if (rc != NAND_OK || ftl_meta_bad(ftl_meta))
        return rc < 0 ? rc : NAND_ERR_ECC;
    return 0;
}

/* Shared by merge batches and unaligned multi-page transfers. Storage's mutex
 * serializes users; aligned reads into this buffer never bounce recursively. */
static uint8_t ftl_transfer[NAND_MAX_PAGES_PER_READ * NAND_PAGE_SIZE] CACHEALIGN_ATTR;
static int ftl_read_pages(uint32_t sector, uint32_t count, void *buffer);
#include "ftl-write.h"

/* Read logical pages (4096 bytes each on the nano 4G) in runs of at most 16
 * within one logical block. Fall back to single-page reads for unmapped
 * pages, unaligned buffers or failed runs. Unwritten pages return 0xff. */

/* (sector and count are FTL pages here, see ftl_read() below) */
static int ftl_read_pages(uint32_t sector, uint32_t count, void *buffer)
{
    uint8_t *buf = buffer;
    uint8_t banks[NAND_MAX_PAGES_PER_READ];
    uint32_t pages[NAND_MAX_PAGES_PER_READ];
    int ret = 0;

    if (!ftl_mounted || sector + count > ftl_pages || sector + count < sector)  /* range and overflow */
        return -1;

    while (count)
    {
        uint32_t off = sector % FTL_PAGES_PER_VB;
        unsigned int n = MIN(count, MIN((uint32_t)NAND_MAX_PAGES_PER_READ,
                                        FTL_PAGES_PER_VB - off));
        unsigned int i;
        _Bool done = false;
        uint8_t *target = buf;
        if ((uintptr_t)buf & (CACHEALIGN_SIZE - 1))
            target = ftl_transfer;
        /* A recovered, unwritten range has no readable data block yet. */
        if (ftl_blank[sector / FTL_PAGES_PER_VB])
        {
            for (i = 0; i < n; i++)
            {
                int rc = ftl_read_current(sector + i,
                                          buf + i * FTL_PAGE_BYTES);
                if (rc)
                    return rc;
            }
            done = true;
        }

        for (i = 0; i < n; i++)
            if (!vfl_locate(ftl_vpn(sector + i), &banks[i], &pages[i]))
                break;

        /* all pages at once if possible; one by one otherwise */
        if (!done && i == n && !((uintptr_t)target & (CACHEALIGN_SIZE - 1))
            && nand_read_pages(n, banks, pages, target, ftl_meta) == NAND_OK)
        {
            done = true;
            if (target != buf)
                memcpy(buf, target, n * FTL_PAGE_BYTES);
            for (i = 0; i < n; i++)
                if (ftl_meta_bad(ftl_meta + i * NAND_META_SIZE))
                    ret = -2;
        }
        for (i = 0; !done && i < n; i++)
        {
            int rc = vfl_read(ftl_vpn(sector + i), buf + i * FTL_PAGE_BYTES,
                              ftl_meta);
            if (rc == NAND_ERR_TIMEOUT || rc == NAND_ERR_NOCHIP)
                return -3;
            if (rc < 0 || (rc == NAND_OK && ftl_meta_bad(ftl_meta)))
                ret = -2;
        }

        ftl_count_reads(sector, n);
        buf += n * FTL_PAGE_BYTES;
        sector += n;
        count -= n;
    }
    return ret;
}

/* Read 4096-byte sectors. Whole pages go straight into the caller's buffer;
 * the parts of the pages at either end of the range pass through a bounce
 * buffer. */
static uint8_t ftl_bounce[NAND_PAGE_SIZE] CACHEALIGN_ATTR;

/* Read-modify-write only for partial physical pages. The storage mutex also
 * protects this buffer, preserving the other 4 KiB half of an 8 KiB page. */
int ftl_write(uint32_t sector, uint32_t count, const void *buffer)
{
    const uint32_t spp = FTL_SECTORS_PER_PAGE;
    const uint8_t *data = buffer;
    if (!ftl_mounted || !ftl_writable || sector > ftl_pages * spp
        || count > ftl_pages * spp - sector || (count && !buffer))
        return NAND_ERR_FTL;
    while (count)
    {
        uint32_t page = sector / spp, sub = sector % spp;
        uint32_t n = MIN(count, spp - sub);
        int rc;
        if (sub == 0 && n == spp)
        {
            n = count / spp * spp;
            rc = ftl_write_pages(page, n / spp, data);
        }
        else
        {
            rc = ftl_read_current(page, ftl_bounce);
            if (rc)
                return rc;
            memcpy(ftl_bounce + sub * 4096, data, n * 4096);
            rc = ftl_write_pages(page, 1, ftl_bounce);
        }
        if (rc)
            return rc;
        sector += n;
        count -= n;
        data += n * 4096;
    }
    return 0;
}

int ftl_read(uint32_t sector, uint32_t count, void *buffer)
{
    const uint32_t spp = FTL_SECTORS_PER_PAGE;
    uint8_t *buf = buffer;
    int ret = 0;

    if (!ftl_mounted || sector + count > ftl_pages * spp
        || sector + count < sector)
        return -1;
    if (spp == 1)
        return ftl_read_pages(sector, count, buffer);

    while (count)
    {
        uint32_t page = sector / spp, sub = sector % spp;
        int rc;

        if (sub == 0 && count >= spp)
        {
            /* whole pages */
            uint32_t n = count / spp;

            rc = ftl_read_pages(page, n, buf);
            if (rc == -1 || rc == -3)
                return rc;
            if (rc)
                ret = rc;
            buf += n * FTL_PAGE_BYTES;
            sector += n * spp;
            count -= n * spp;
        }
        else
        {
            /* part of a page */
            uint32_t n = MIN(count, spp - sub);

            rc = ftl_read_pages(page, 1, ftl_bounce);
            if (rc == -1 || rc == -3)
                return rc;
            if (rc)
                ret = rc;
            memcpy(buf, ftl_bounce + sub * 4096, n * 4096);
            buf += n * 4096;
            sector += n;
            count -= n;
        }
    }
    return ret;
}
