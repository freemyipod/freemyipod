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

/* bank indexes populated chip enables; page is the page index within that chip.
 * Each page has 4096 or 8192 data bytes and 12 metadata bytes: 10 protected bytes
 * followed by two 0xff bytes. */
#ifndef __NAND_TARGET_H__
#define __NAND_TARGET_H__

#include "libc/include/stdint.h"

/* Largest configuration we drive: up to 8 chip enables (the count READ ID
 * scans), 8192 blocks per CE, 128 pages per block, up to 8192-byte pages */
#define NAND_MAX_BANKS          8
/* The nano 5G parts have 4096- or 8192-byte pages; the page size in use is
 * nand_geometry.page_size. NAND_PAGE_SIZE is the largest one, for buffers.
 */
#define NAND_PAGE_SIZE          8192
#define NAND_META_SIZE          12      /* 10 ECC-protected bytes + 0xff 0xff */

/* Pages one sequencer run may read (bounded by the static request lists) */
#define NAND_MAX_PAGES_PER_READ 16

/* Results of a page read */
#define NAND_OK                 0
#define NAND_ERASED             1       /* the page was never written */
#define NAND_ERR_ECC            (-2)    /* at least one uncorrectable sector */
#define NAND_ERR_TIMEOUT        (-3)    /* sequencer hung or faulted */
#define NAND_ERR_NOCHIP         (-4)    /* no or unknown flash chips */
#define NAND_ERR_UNSUPPORTED    (-5)    /* chips known, layout not driven */
#define NAND_ERR_FTL            (-6)    /* FTL could not be mounted */
#define NAND_ERR_WRITE          (-7)    /* NAND program or erase failed */

/* Vendor classes stored in the FTL context. */
#define NAND_VENDOR_TOSHIBA     1
#define NAND_VENDOR_SAMSUNG     2
#define NAND_VENDOR_HYNIX       3
#define NAND_VENDOR_MICRON      5
#define NAND_VENDOR_INTEL       6
#define NAND_VENDOR_MICRON_2    7
#define NAND_VENDOR_SAMSUNG_8K  8       /* Samsung parts with 8192-byte pages */

/* Address layout of the FTL volume over the chips, the word the nano 4G
 * firmware configures its VFL with for a population. Low 16 bits: how
 * logical "ways" map to chips (1 = one way per chip enable, the identity
 * the FTL here implements; 0x10..0x14 = two ways per chip, a scheme of the
 * two-chip populations); high 16 bits: a program-command variant used by
 * the write path only. */
#define NAND_LAYOUT_1WAY        0x00010001  /* every four-chip population */
#define NAND_LAYOUT_2WAY_14     0x00100014  /* 2 x Samsung / Hynix 4096-block parts */
#define NAND_LAYOUT_2WAY_11_12  0x00110012  /* 2 x Intel / Micron 8192-block parts */
#define NAND_LAYOUT_2WAY_12_14  0x00120014  /* 2 x Intel / Micron 4096-block parts and 2c d7 94 3e */

struct nand_geometry
{
    uint32_t id;                /* READ ID bytes 0..3, little endian */
    unsigned int banks;         /* populated chip enables */
    uint8_t ce[NAND_MAX_BANKS]; /* chip enable of bank 0..banks-1 */
    unsigned int blocks;        /* per chip enable */
    unsigned int pages_per_block;  /* 128 for every part listed */
    unsigned int page_size;     /* bytes */
    unsigned int spare_size;    /* bytes */
    unsigned int ecc_bits;      /* 8 or 16 per 512-byte sector */
    unsigned int vendor;        /* NAND_VENDOR_* */
    uint32_t layout;            /* NAND_LAYOUT_* of the population */
    /* Per-die addressing and refresh geometry. */
    unsigned int dies;          /* dies per chip enable (1 or 2) */
    unsigned int blocks_per_die;
    unsigned int die_stride;    /* row-address blocks between consecutive dies */
    unsigned int block_span;    /* address span, including holes between dies */
    unsigned int blocks_aligned; /* usable nominal blocks, rounded down to 4096 */
    unsigned int page_stride;   /* row-address pages between consecutive blocks */
    unsigned int refresh_threshold;
};

/* The four READ ID bytes, decoded for display. Bytes 3 and 4 follow the
 * legacy (pre-ONFI) layout of Samsung/Hynix datasheets of that generation,
 * the one Linux decodes in nand_decode_ext_id(); other vendors deviate
 * (Toshiba's 218-byte-spare parts say 8 bytes per 512), so the geometry
 * the driver uses comes from its part table, never from these bits. */
struct nand_id
{
    uint8_t maker;              /* byte 1: 0xad Hynix, 0x98 Toshiba, ... */
    uint8_t device;             /* byte 2: device code */
    uint8_t byte3, byte4;       /* raw bytes 3 and 4 */
    const char *maker_name;
    /* byte 3 */
    uint8_t dies_per_ce;        /* bits 1..0: 1, 2, 4, 8 */
    uint8_t cell_levels;        /* bits 3..2: 2 (SLC), 4 (MLC), 8, 16 */
    uint8_t planes;             /* bits 5..4: pages programmed at once */
    /* Preserve the one-byte flags in the decoded chip ID. */
    _Bool interleave;            /* bit 6: interleaved program between dies */
    _Bool cache_program;         /* bit 7 */
    /* byte 4 */
    uint16_t page_size;         /* bits 1..0: 1, 2, 4, 8 KiB */
    uint8_t spare_per_512;      /* bit 2: 8 or 16 bytes per 512 */
    uint16_t block_kib;         /* bits 5..4: 64, 128, 256, 512 KiB */
    uint8_t bus_width;          /* bit 6: 8 or 16 */
};

void nand_decode_id(uint32_t id, struct nand_id *out);

/* Decoded ID of the first chip found by nand_phys_init() (also set when
 * the chip is not supported), NULL before detection or when no chip enable
 * answered */
const struct nand_id *nand_get_id(void);

/* READ ID bytes 0..3 of CE0..CE7 from the last detection (0 or 0xffffffff
 * for an empty chip enable), NULL before one ran; for diagnostics */
const uint32_t *nand_get_raw_ids(void);

/* Reset the controller, detect the chips and set the timing. */
int nand_phys_init(void);

const struct nand_geometry *nand_get_geometry(void);

/* Page number (row address) of page 0 of a block. The dies of a chip enable
 * are mapped one after another, each starting at a power-of-two multiple of
 * its block count, so blocks above the first die skip the unused part of
 * the previous die's address range. */
uint32_t nand_block_row(unsigned int block);

/* Read one page. meta receives NAND_META_SIZE bytes (all 0xff for an erased
 * page) and may be NULL. data must be 32-byte aligned or it is bounced.
 * Returns NAND_OK, NAND_ERASED or a negative NAND_ERR_*. A timeout or ECC
 * failure is retried once after a chip reset. */
int nand_read_page(unsigned int bank, uint32_t page, void *data, void *meta);

/* Read n (<= NAND_MAX_PAGES_PER_READ) pages in one sequencer run. Page i is
 * (bank[i], page[i]) and goes to data + i * NAND_PAGE_SIZE (32-byte aligned),
 * its meta to meta + i * NAND_META_SIZE. Array reads overlap across independent
 * chip enables; data is drained in caller order. Returns NAND_OK if every page was read without an
 * uncorrectable sector (erased pages count as read and contain 0xff),
 * otherwise a negative NAND_ERR_* and the caller should fall back to
 * nand_read_page() per page. */
int nand_read_pages(unsigned int n, const uint8_t *bank, const uint32_t *page,
                    void *data, void *meta);

_Bool nand_write_supported(void);
/* Physical CE bit mask from the last completed program/erase, before reset. */
unsigned int nand_get_last_failure_mask(void);
/* Batches contain at most NAND_MAX_PAGES_PER_READ pages, using the runtime
 * page size. Batch data must be cache-line aligned. All rows are validated
 * before a mutation; calls require the same external locking as page reads. */
int nand_program_pages(unsigned int count, const uint8_t *banks,
                       const uint32_t *rows, const void *data, const void *meta);
int nand_program_sequential(unsigned int count, unsigned int planes,
                            const uint32_t *row_base, const void *data,
                            const void *meta, _Bool aligned, _Bool correct_bank);
int nand_erase_blocks(unsigned int count, const uint8_t *banks,
                      const uint32_t *blocks);
int nand_read_boot_page(unsigned int bank, uint32_t row, void *data);
int nand_program_boot_page(unsigned int bank, uint32_t row, const void *data);
/* Raw transfers preserve OSOS semantics: write waits for READY but does not
 * inspect NAND FAIL. Buffers hold page_size data and spare_size spare bytes. */
int nand_transfer_raw(unsigned int bank, uint32_t row, void *data,
                      void *spare, _Bool write);
/* Program an erased page or erase a block. The caller serialises these
 * operations with reads. A failed mutation is not retried in place.
 * A NULL program metadata pointer stores ten 0xa5 bytes, like OSOS. */
int nand_device_program(unsigned int bank, uint32_t page,
                        const void *data, const void *meta);
int nand_device_erase(unsigned int bank, uint32_t block);
/* Diagnostics: status byte of a bank's chip (bit 7 = not write protected), or -1 */
int nand_read_chip_status(unsigned int bank);

/* Pages read since boot (diagnostics) */
uint32_t nand_get_read_count(void);

#endif /* __NAND_TARGET_H__ */
