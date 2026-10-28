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

/* Private write half of the virtual flash layer. Included by ftl-nano5g.c
 * in the main firmware only. The storage mutex protects these buffers too.
 *
 * A program failure does not change the page's address: other pages in that
 * block may still be live. It records a pending replacement instead. Only
 * the owner of a free virtual block may erase it and install a replacement.
 * Changes to the address map become durable before an erase reports success.
 */
#define VFL_W_SEQUENCE       0x004
#define VFL_W_ACTIVE         0x00c
#define VFL_W_NEXT           0x00e
#define VFL_W_ERASE_FAILURES 0x014
#define VFL_W_RESERVE_CURSOR 0x016
#define VFL_W_SCRUB_COUNT    0x6a0
#define VFL_W_SCRUB_LIST     0x6a2
#define VFL_W_SCRUB_LIMIT    20

static _Bool vfl_write_failed;
static uint8_t vfl_write_page[NAND_PAGE_SIZE] CACHEALIGN_ATTR;
static uint8_t vfl_verify_page[NAND_PAGE_SIZE] CACHEALIGN_ATTR;
static uint8_t vfl_write_meta[NAND_META_SIZE] __attribute__((aligned(4)));
static uint8_t vfl_verify_meta[NAND_META_SIZE] __attribute__((aligned(4)));

static _Bool vfl_context_read_group(unsigned int bank, unsigned int block,
                                   unsigned int page)
{
    unsigned int i;

    for (i = 0; i < VFL_CXT_COPIES; i++)
        if (nand_read_page(bank, FTL_ROW(block, page + i),
                           vfl_verify_page, vfl_verify_meta) == NAND_OK &&
            vfl_verify_meta[8] == 0 && vfl_verify_meta[9] == 0x80)
            return true;
    return false;
}

/* Context writes must not reuse the caller's data or metadata buffers. */
static int vfl_write_init(void)
{
    unsigned int bank, i, j;

    vfl_write_failed = true;
    for (bank = 0; bank < FTL_BANKS; bank++)
    {
        uint8_t *c = vfl_cxt[bank];
        unsigned int active = 4, next = 0;
        uint32_t sequence = UINT32_MAX;
        unsigned int count = get16(c + VFL_W_SCRUB_COUNT);

        if (count > VFL_W_SCRUB_LIMIT)
            return NAND_ERR_FTL;
        for (i = 0; i < vfl_blocks - vfl_pool_start * VFL_WAYS; i++)
        {
            unsigned int target = vfl_remap[bank][i];
            if (target == VFL_REMAP_UNUSED || target == VFL_REMAP_BAD)
                continue;
            if (target >= vfl_blocks)
                return NAND_ERR_FTL;
            for (j = 0; j < i; j++)
                if (target == vfl_remap[bank][j])
                    return NAND_ERR_FTL;
        }
        for (i = 0; i < 4; i++)
        {
            unsigned int b = get16(c + VFL_CXT_BLOCKS + 2 * i);
            if (b == 0xffff)
                continue;
            if (!b || b > VFL_SCAN_LAST || b >= vfl_pool_start)
                return NAND_ERR_FTL;
            for (j = 0; j < i; j++)
                if (b == get16(c + VFL_CXT_BLOCKS + 2 * j))
                    return NAND_ERR_FTL;
            if (vfl_context_read_group(bank, b, 0))
            {
                uint32_t candidate = get32(vfl_verify_meta);
                if (candidate && candidate <= sequence)
                {
                    sequence = candidate;
                    active = i;
                }
            }
        }
        if (active == 4)
            return NAND_ERR_FTL;
        /* Recover the append cursor from the flash itself. A recovered
         * volume need not have trustworthy cursor fields in its payload. */
        for (i = 0; i < FTL_PAGES_PER_BLOCK; i += VFL_CXT_COPIES)
        {
            unsigned int block = get16(c + VFL_CXT_BLOCKS + 2 * active);
            if (!vfl_context_read_group(bank, block, i))
                break;
            sequence = get32(vfl_verify_meta);
            next = i + VFL_CXT_COPIES;
        }
        put16(c + VFL_W_ACTIVE, active);
        put16(c + VFL_W_NEXT, next);
        put32(c + VFL_W_SEQUENCE, sequence);
        for (i = 0; i < count; i++)
            if (get16(c + VFL_W_SCRUB_LIST + 2 * i) >= vfl_blocks)
                return NAND_ERR_FTL;
    }
    vfl_write_failed = false;
    return NAND_OK;
}

static int vfl_write_stop(void)
{
    /* A partially persisted address change cannot be rolled back safely.
     * Reopen the volume before allowing any more changes. */
    vfl_write_failed = true;
    return NAND_ERR_FTL;
}

/* A controller timeout or a missing chip says nothing about the block:
 * stop writing instead of retiring blocks or spending spares. */
static _Bool vfl_controller_fault(int rc)
{
    if (rc != NAND_ERR_TIMEOUT && rc != NAND_ERR_NOCHIP)
        return false;
    vfl_write_failed = true;
    return true;
}

static void vfl_update_checksum(uint8_t *c)
{
    unsigned int i;
    uint32_t sum = 0, parity = 0;

    for (i = 0; i < VFL_CXT_SUM1; i += 4)
    {
        uint32_t word = get32(c + i);
        sum += word;
        parity ^= word;
    }
    put32(c + VFL_CXT_SUM1, sum + 0xaabbccdd);
    put32(c + VFL_CXT_SUM2, parity ^ 0xaabbccdd);
}

static _Bool vfl_context_append(unsigned int bank)
{
    uint8_t *c = vfl_cxt[bank];
    unsigned int i, good = 0;
    unsigned int block = get16(c + VFL_CXT_BLOCKS +
                              2 * get16(c + VFL_W_ACTIVE));
    unsigned int next = get16(c + VFL_W_NEXT);
    uint32_t sequence = get32(c + VFL_W_SEQUENCE) - 1;
    uint32_t generation = 0;
    uint32_t row = FTL_ROW(block, next);

    if (!sequence || next + VFL_CXT_COPIES > FTL_PAGES_PER_BLOCK)
        return false;
    /* Do not trust a stale cursor enough to program over existing data.
     * This also handles an interrupted append with only some copies written. */
    for (i = 0; i < VFL_CXT_COPIES; i++)
        if (nand_read_page(bank, row + i, vfl_verify_page, vfl_verify_meta)
            != NAND_ERASED)
            return false;
    for (i = 0; i < FTL_BANKS; i++)
        if (get32(vfl_cxt[i] + VFL_CXT_USN) > generation)
            generation = get32(vfl_cxt[i] + VFL_CXT_USN);
    if (generation == UINT32_MAX)
        return false;

    put32(c + VFL_CXT_USN, generation + 1);
    put32(c + VFL_W_SEQUENCE, sequence);
    put16(c + VFL_W_NEXT, next + VFL_CXT_COPIES);
    /* Each bank has its own remap table, but these fields are volume-wide.
     * Copy them from the values selected at mount, not an older bank copy. */
    put16(c + VFL_CXT_USABLE, vfl_usable);
    put16(c + VFL_CXT_POOL_START, vfl_pool_start);
    for (i = 0; i < 3; i++)
        put16(c + VFL_CXT_FTL_CTRL + 2 * i, vfl_ftl_ctrl[i]);
    vfl_update_checksum(c);
    memset(vfl_write_page, 0xff, sizeof(vfl_write_page));
    memcpy(vfl_write_page, c, VFL_CXT_SIZE);
    memset(vfl_write_meta, 0xff, sizeof(vfl_write_meta));
    put32(vfl_write_meta, sequence);
    vfl_write_meta[8] = 0;
    vfl_write_meta[9] = 0x80;
    for (i = 0; i < VFL_CXT_COPIES; i++)
    {
        int rc = nand_device_program(bank, row + i, vfl_write_page, vfl_write_meta);
        vfl_controller_fault(rc);
        if (rc != NAND_OK)
            return false;
    }
    for (i = 0; i < VFL_CXT_COPIES; i++)
        if (nand_read_page(bank, row + i, vfl_verify_page, vfl_verify_meta)
            == NAND_OK && !memcmp(vfl_verify_page, c, VFL_CXT_SIZE) &&
            !memcmp(vfl_verify_meta, vfl_write_meta, NAND_META_SIZE))
            good++;
    return good >= 5;
}

static int vfl_store_context(unsigned int bank)
{
    uint8_t *c = vfl_cxt[bank];
    unsigned int original = get16(c + VFL_W_ACTIVE);
    unsigned int step;

    if (vfl_write_failed)
        return NAND_ERR_FTL;
    if (vfl_context_append(bank))
        return NAND_OK;
    /* Keep the original block intact, even if every alternate block fails.
     * There may already be readable copies of the new context in it. */
    for (step = 1; step < 4; step++)
    {
        unsigned int slot = (original + step) % 4;
        unsigned int block = get16(c + VFL_CXT_BLOCKS + 2 * slot);
        if (vfl_write_failed)
            return NAND_ERR_FTL;
        if (block == 0xffff)
            continue;
        int rc = nand_device_erase(bank, block);
        if (vfl_controller_fault(rc))
            return NAND_ERR_FTL;
        if (rc != NAND_OK)
            continue;
        put16(c + VFL_W_ACTIVE, slot);
        put16(c + VFL_W_NEXT, 0);
        if (vfl_context_append(bank))
            return NAND_OK;
    }
    return vfl_write_stop();
}

/* Restore OS 0x0803a624, called by open at 0x08042484 before FTL open.
 * A zero requested size means all blocks below the reserve pool. Leave
 * the pending record intact: Apple's format/type setter clears it later.
 * Unlike Apple's caller, propagate a context-store failure to the mount. */
static int vfl_apply_pending_geometry(void)
{
    uint8_t *record = vfl_cxt[0] + 0x6ca;
    uint32_t generation = 0;
    unsigned int bank, usable;

    if (!record[15])
        return NAND_OK;
    usable = get16(record + 4);
    if (!usable)
        usable = vfl_pool_start;
    if (usable <= FTL_EXTRA_VBS || usable > vfl_pool_start)
        return NAND_ERR_FTL;
    for (bank = 0; bank < FTL_BANKS; bank++)
        if (get32(vfl_cxt[bank] + VFL_CXT_USN) > generation)
            generation = get32(vfl_cxt[bank] + VFL_CXT_USN);
    vfl_usable = usable;
    for (bank = 0; bank < FTL_BANKS; bank++)
    {
        uint8_t *c = vfl_cxt[bank];
        put32(c + VFL_CXT_FTL_TYPE, get32(record));
        put16(c + VFL_CXT_USABLE, usable);
        vfl_update_checksum(c);
    }
    return vfl_store_context(generation % FTL_BANKS);
}

static int vfl_scrub_index(unsigned int bank, unsigned int vb)
{
    const uint8_t *c = vfl_cxt[bank];
    unsigned int i, count = get16(c + VFL_W_SCRUB_COUNT);

    for (i = 0; i < count; i++)
        if (get16(c + VFL_W_SCRUB_LIST + 2 * i) == vb)
            return i;
    return -1;
}

static int vfl_queue_replacement(unsigned int bank, unsigned int vb)
{
    uint8_t *c = vfl_cxt[bank];
    unsigned int count = get16(c + VFL_W_SCRUB_COUNT);

    if (vfl_scrub_index(bank, vb) >= 0)
        return NAND_OK;
    if (count >= VFL_W_SCRUB_LIMIT)
        return vfl_write_stop();
    put16(c + VFL_W_SCRUB_LIST + 2 * count, vb);
    put16(c + VFL_W_SCRUB_COUNT, count + 1);
    return vfl_store_context(bank);
}

static void vfl_clear_replacement(unsigned int bank, unsigned int vb)
{
    uint8_t *c = vfl_cxt[bank];
    int index = vfl_scrub_index(bank, vb);

    if (index >= 0)
    {
        unsigned int count = get16(c + VFL_W_SCRUB_COUNT) - 1;
        put16(c + VFL_W_SCRUB_LIST + 2 * index,
              get16(c + VFL_W_SCRUB_LIST + 2 * count));
        put16(c + VFL_W_SCRUB_LIST + 2 * count, 0);
        put16(c + VFL_W_SCRUB_COUNT, count);
    }
}

static void vfl_set_remap(unsigned int bank, unsigned int index, uint16_t value)
{
    vfl_remap[bank][index] = value;
    put16(vfl_cxt[bank] + VFL_CXT_REMAP + 2 * index, value);
}

/* Select and erase a spare before publishing the new address. No other
 * live virtual block or physical context block is touched here. */
static int vfl_replace_block(unsigned int bank, unsigned int vb)
{
    uint8_t *c = vfl_cxt[bank];
    unsigned int i, j, entries = vfl_blocks - vfl_pool_start * VFL_WAYS;

    for (i = 0; i < entries; i++)
    {
        unsigned int physical = vfl_reserve_block(bank, i);
        if (vfl_remap[bank][i] != VFL_REMAP_UNUSED)
            continue;
        int rc = nand_device_erase(bank, physical);
        if (vfl_controller_fault(rc))
            return NAND_ERR_FTL;
        if (rc != NAND_OK)
        {
            vfl_set_remap(bank, i, VFL_REMAP_BAD);
            vfl_bad[bank][physical >> 3] |= 1 << (physical & 7);
            continue;
        }
        for (j = 0; j < entries; j++)
            if (vfl_remap[bank][j] == vb)
            {
                unsigned int old = vfl_reserve_block(bank, j);
                vfl_set_remap(bank, j, VFL_REMAP_BAD);
                vfl_bad[bank][old >> 3] |= 1 << (old & 7);
            }
        vfl_set_remap(bank, i, vb);
        vfl_bad[bank][vb >> 3] |= 1 << (vb & 7);
        unsigned int per = vfl_blocks / VFL_WAYS - vfl_pool_start;
        put16(c + VFL_W_RESERVE_CURSOR + 2 * (i / per), i % per + 1);
        vfl_clear_replacement(bank, vb);
        return vfl_store_context(bank);
    }
    /* Preserve any newly discovered bad reserve entries if possible. */
    vfl_store_context(bank);
    return vfl_write_stop();
}

static _Bool vfl_data_block(unsigned int bank, unsigned int physical)
{
    unsigned int i;

    /* Block zero can be a mapped FTL data block. Protect the actual context
     * blocks below, rather than rejecting an otherwise valid data address. */
    if (physical >= vfl_blocks)
        return false;
    for (i = 0; i < 4; i++)
        if (physical == get16(vfl_cxt[bank] + VFL_CXT_BLOCKS + 2 * i))
            return false;
    return true;
}

static int vfl_program(uint32_t vpn, const void *data, const uint8_t *meta)
{
    uint8_t bank;
    uint32_t page;
    int rc;

    if (vfl_write_failed || vpn / FTL_PAGES_PER_VB >= vfl_usable ||
        !data || !meta || !vfl_locate(vpn, &bank, &page) ||
        !vfl_data_block(bank, vfl_remap_block(bank,
            vfl_c2p(vpn % FTL_VBANKS, vpn / FTL_PAGES_PER_VB))))
        return NAND_ERR_FTL;
    rc = nand_device_program(bank, page, data, meta);
    if (vfl_controller_fault(rc))
        return NAND_ERR_FTL;
    if (rc != NAND_OK &&
        vfl_queue_replacement(bank, vfl_c2p(vpn % FTL_VBANKS, vpn / FTL_PAGES_PER_VB)) != NAND_OK)
        return NAND_ERR_FTL;
    return rc;
}

/* Submit one bounded, consecutive virtual-page run. Mapping and protection
 * checks finish before NAND is touched. Physical requests retain caller order;
 * the existing scattered planner overlaps programming on independent CEs. */
static int vfl_program_pages(uint32_t vpn, unsigned int count,
                              const void *data, const uint8_t *meta)
{
    uint8_t banks[NAND_MAX_PAGES_PER_READ];
    uint32_t rows[NAND_MAX_PAGES_PER_READ];
    unsigned int vb = vpn / FTL_PAGES_PER_VB;
    if (vfl_write_failed || !count || count > NAND_MAX_PAGES_PER_READ ||
        vb >= vfl_usable || count > FTL_PAGES_PER_VB - vpn % FTL_PAGES_PER_VB ||
        !data || !meta || (uintptr_t)data % CACHEALIGN_SIZE)
        return NAND_ERR_FTL;
    for (unsigned int i = 0; i < count; i++)
        if (!vfl_locate(vpn + i, &banks[i], &rows[i]) ||
            !vfl_data_block(banks[i], vfl_remap_block(banks[i],
                vfl_c2p((vpn + i) % FTL_VBANKS, vb))))
            return NAND_ERR_FTL;
    int rc = nand_program_pages(count, banks, rows, data, meta);
    if (vfl_controller_fault(rc))
        return NAND_ERR_FTL;
    if (rc != NAND_OK)
    {
        unsigned int failed = nand_get_last_failure_mask();
        const struct nand_geometry *geometry = nand_get_geometry();
        for (unsigned int i = 0; i < count; i++)
            if ((!failed || (failed & (1u << geometry->ce[banks[i]]))) &&
                vfl_queue_replacement(banks[i],
                    vfl_c2p((vpn + i) % FTL_VBANKS, vb)) != NAND_OK)
                return NAND_ERR_FTL;
    }
    return rc;
}

static int vfl_erase(unsigned int vb)
{
    unsigned int vbank;

    if (vfl_write_failed || vb >= vfl_usable)
        return NAND_ERR_FTL;
    for (vbank = 0; vbank < FTL_VBANKS; vbank++)
    {
        unsigned int bank = vbank % FTL_BANKS;
        unsigned int nominal = vfl_c2p(vbank, vb);
        unsigned int physical = vfl_remap_block(bank, nominal);
        if (vfl_scrub_index(bank, nominal) < 0)
        {
            uint8_t *c = vfl_cxt[bank];
            if (!vfl_data_block(bank, physical))
                return vfl_write_stop();
            int rc = nand_device_erase(bank, physical);
            if (vfl_controller_fault(rc))
                return NAND_ERR_FTL;
            if (rc == NAND_OK)
                continue;
            put16(c + VFL_W_ERASE_FAILURES,
                  get16(c + VFL_W_ERASE_FAILURES) + 1);
        }
        if (vfl_replace_block(bank, nominal) != NAND_OK)
            return NAND_ERR_FTL;
    }
    return NAND_OK;
}
