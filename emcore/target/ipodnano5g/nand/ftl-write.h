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

/* Private write implementation for ftl-nano5g.c.
 *
 * This writer uses the existing disk format with its own allocation policy:
 * append changed pages to a log, then consolidate into an erased block when
 * a random log fills. A complete sequential log becomes the data block without
 * copying it. Only the final, separately completed page carries the data-block
 * tag. Until that page succeeds the source blocks remain intact.
 *
 * The on-flash checkpoint is invalidated BEFORE changing data. A checkpoint
 * consists of complete tables followed by a context page, so interruption
 * causes a metadata scan instead of exposing half of a new block map.
 * Free blocks are erased before publishing a clean checkpoint, as required
 * by the persistent format. No RAM pointers go on disk.
 */

#define BLOCK_AVAILABLE 0
#define BLOCK_OWNED     1
#define BLOCK_ERASED    2
#define FTL_READ_REFRESH 10000
#define FTL_WEAR_SPREAD  32u

static uint8_t block_owner[FTL_MAX_BLOCKS];
static uint16_t erase_count[FTL_MAX_BLOCKS];
static uint16_t read_count[FTL_MAX_BLOCKS];
static uint16_t log_next[FTL_LOG_SLOTS];
static uint32_t data_sequence, control_sequence, control_next;
static unsigned int control_index, allocation_cursor;
static unsigned int merge_count;
static _Bool write_dirty, write_failed;
static uint8_t write_stats[NAND_PAGE_SIZE] CACHEALIGN_ATTR;

static int ftl_write_error(int rc)
{
    /* A timeout does not tell us whether NAND accepted a program. Do not
     * reuse that page, erase its sources, or publish a checkpoint afterward. */
    write_failed = true;
    return rc < 0 ? rc : NAND_ERR_FTL;
}

static _Bool ftl_claim(unsigned int vb)
{
    if (vb >= vfl_usable || block_owner[vb] == BLOCK_OWNED)
        return false;
    block_owner[vb] = BLOCK_OWNED;
    return true;
}

/* Recover the cursor from media, not a possibly interrupted RAM snapshot.
 * Unknown pages count as occupied and are never programmed again. */
static int ftl_write_mount(void)
{
    unsigned int i, j, selected = 3, highest = 0;
    uint32_t best = UINT32_MAX;
    uint32_t ec_seq[(FTL_MAX_BLOCKS * 2 + 4095) / 4096];
    uint32_t rc_seq[(FTL_MAX_BLOCKS * 2 + 4095) / 4096];

    write_failed = write_dirty = false;
    data_sequence = 0;
    control_sequence = UINT32_MAX;
    control_next = FTL_PAGES_PER_VB;
    control_index = allocation_cursor = merge_count = 0;
    memset(block_owner, 0, sizeof(block_owner));
    memset(erase_count, 0, sizeof(erase_count));
    memset(read_count, 0, sizeof(read_count));
    memset(write_stats, 0, sizeof(write_stats));
    memset(ec_seq, 0xff, sizeof(ec_seq));
    memset(rc_seq, 0xff, sizeof(rc_seq));

    for (i = 0; i < 3; i++)
        if (!ftl_claim(vfl_ftl_ctrl[i]))
            return NAND_ERR_FTL;
    for (i = 0; i < ftl_vbs; i++)
    {
        if (!ftl_claim(ftl_map[i]))
            return NAND_ERR_FTL;
        if (ftl_blank[i])
            continue;
        for (j = 0; j < 2; j++)
        {
            uint32_t vpn = ftl_map[i] * FTL_PAGES_PER_VB
                         + (j ? FTL_PAGES_PER_VB - 1 : 0);
            int rc = vfl_read(vpn, ftl_buf, ftl_meta);
            if (rc == NAND_ERR_TIMEOUT || rc == NAND_ERR_NOCHIP)
                return rc;
            if (rc == NAND_OK && (ftl_meta[9] == META_DATA
                                  || ftl_meta[9] == META_LOG))
                data_sequence = MAX(data_sequence, get32(ftl_meta + 4));
        }
    }
    if (ftl_logs[FTL_LOG_SLOTS].vbn != 0xffff)
        return NAND_ERR_FTL;
    for (i = 0; i < FTL_LOG_SLOTS; i++)
    {
        struct ftl_log *log = &ftl_logs[i];
        /* Never append to a log opened by another session: an erased-looking
         * tail may contain an interrupted NAND program. */
        log_next[i] = FTL_PAGES_PER_VB;
        if (log->vbn == 0xffff)
            continue;
        if (log->lbn >= ftl_vbs || !ftl_claim(log->vbn))
            return NAND_ERR_FTL;
        for (j = 0; j < i; j++)
            if (ftl_logs[j].vbn != 0xffff && ftl_logs[j].lbn == log->lbn)
                return NAND_ERR_FTL;
        data_sequence = MAX(data_sequence, log->usn);
        for (j = 0; j < FTL_PAGES_PER_VB; j++)
            if (FTL_OFFS(i, j) != 0xffff
                && FTL_OFFS(i, j) >= FTL_PAGES_PER_VB)
                return NAND_ERR_FTL;
    }

    if (!ftl_restored)
    {
        data_sequence = MAX(data_sequence, get32(ftl_cxt + 4));
        if (!ftl_read_table(FTL_CXT_EC_PAGES,
                            (vfl_usable * 2 + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES,
                            erase_count, vfl_usable * 2)
            || !ftl_read_table(FTL_CXT_RC_PAGES,
                            (vfl_usable * 2 + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES,
                            read_count, vfl_usable * 2))
            return NAND_ERR_FTL;
        if (get32(ftl_cxt + FTL_CXT_STATS_FLAG) == UINT32_MAX
            && vfl_read(get32(ftl_cxt + FTL_CXT_STATS_PAGE),
                        write_stats, ftl_meta) != NAND_OK)
            memset(write_stats, 0, sizeof(write_stats));
    }

    for (i = 0; i < 3; i++)
    {
        unsigned int last = 0;
        uint32_t first = UINT32_MAX;
        for (j = 0; j < FTL_PAGES_PER_VB; j++)
        {
            int rc = vfl_read(vfl_ftl_ctrl[i] * FTL_PAGES_PER_VB + j,
                              ftl_buf, ftl_meta);
            uint32_t seq, idx;
            if (rc == NAND_ERR_TIMEOUT || rc == NAND_ERR_NOCHIP)
                return rc;
            if (rc != NAND_ERASED)
                last = j + 1;
            if (rc != NAND_OK || !ftl_is_ctrl_type(ftl_meta[9]))
                continue;
            seq = get32(ftl_meta);
            control_sequence = MIN(control_sequence, seq);
            if (j == 0)
                first = seq;
            /* Keep wear history across a dirty mount when readable table
             * pages survive. Each table fragment has its own generation. */
            idx = get32(ftl_meta + 4);
            if (!ftl_restored || idx >= (sizeof(erase_count) + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES)
                continue;
            uint16_t *table = NULL;
            if (ftl_meta[9] == 'F' && seq <= ec_seq[idx])
            {
                ec_seq[idx] = seq;
                table = erase_count;
            }
            if (ftl_meta[9] == 'I' && seq <= rc_seq[idx])
            {
                rc_seq[idx] = seq;
                table = read_count;
            }
            if (table)
                memcpy((uint8_t *)table + idx * FTL_PAGE_BYTES,
                       ftl_buf, MIN(FTL_PAGE_BYTES,
                           sizeof(erase_count) - idx * FTL_PAGE_BYTES));
        }
        if (first < best)
        {
            best = first;
            selected = i;
            highest = last;
        }
    }
    if (selected != 3)
    {
        control_index = selected;
        control_next = highest;
    }
    return 0;
}

static int ftl_erase_block(unsigned int vb)
{
    int rc = vfl_erase(vb);
    if (rc)
        return ftl_write_error(rc);
    if (erase_count[vb] != UINT16_MAX)
        erase_count[vb]++;
    read_count[vb] = 0;
    return 0;
}

/* Rotate equal-wear candidates, favouring less-used blocks. No block may be
 * allocated until the on-flash context has been invalidated. */
static int ftl_allocate(void)
{
    unsigned int best = vfl_usable, i;
    for (i = 0; i < vfl_usable; i++)
    {
        unsigned int vb = (allocation_cursor + i) % vfl_usable;
        if (block_owner[vb] != BLOCK_OWNED
            && (best == vfl_usable || erase_count[vb] < erase_count[best]))
            best = vb;
    }
    if (best == vfl_usable)
        return ftl_write_error(NAND_ERR_FTL);
    if (block_owner[best] != BLOCK_ERASED && ftl_erase_block(best))
        return NAND_ERR_FTL;
    block_owner[best] = BLOCK_OWNED;
    allocation_cursor = (best + 1) % vfl_usable;
    return best;
}

static int ftl_control_space(unsigned int needed)
{
    unsigned int next;
    int rc;
    if (control_next + needed <= FTL_PAGES_PER_VB)
        return 0;
    next = (control_index + 1) % 3;
    rc = ftl_erase_block(vfl_ftl_ctrl[next]);
    if (rc)
        return rc;
    control_index = next;
    control_next = 0;
    return 0;
}

static int ftl_control_page(uint8_t type, uint32_t idx, const void *data)
{
    uint8_t meta[NAND_META_SIZE];
    int rc;
    memset(meta, 0xff, sizeof(meta));
    put32(meta, control_sequence);
    put32(meta + 4, idx);
    meta[9] = type;
    rc = vfl_program(vfl_ftl_ctrl[control_index] * FTL_PAGES_PER_VB
                     + control_next, data, meta);
    control_next++;
    return rc ? ftl_write_error(rc) : 0;
}

static int ftl_begin_write(void)
{
    unsigned int lbn;
    int rc;
    if (write_failed)
        return NAND_ERR_FTL;
    if (write_dirty)
        return 0;
    if (control_sequence < 2 || data_sequence == UINT32_MAX)
        return ftl_write_error(NAND_ERR_FTL);
    rc = ftl_control_space(1);
    if (rc)
        return rc;
    control_sequence--;
    memset(ftl_buf, 0xff, sizeof(ftl_buf));
    rc = ftl_control_page('O', UINT32_MAX, ftl_buf);
    if (rc)
        return rc;
    write_dirty = true;
    /* Recovery can assign an unclaimed, stale block to an unwritten LBN.
     * Erase these only after the invalid marker is durable. */
    for (lbn = 0; lbn < ftl_vbs; lbn++)
        if (ftl_blank[lbn])
        {
            rc = ftl_erase_block(ftl_map[lbn]);
            if (rc)
                return rc;
            ftl_blank[lbn] = false;
        }
    return 0;
}

static int ftl_data_pages(unsigned int vb, unsigned int off, uint32_t lpn,
                          uint32_t sequence, unsigned int count,
                          uint8_t last_type, const void *data)
{
    uint8_t meta[NAND_MAX_PAGES_PER_READ * NAND_META_SIZE];
    int rc;
    memset(meta, 0xff, count * NAND_META_SIZE);
    for (unsigned int i = 0; i < count; i++)
    {
        put32(meta + i * NAND_META_SIZE, lpn + i);
        put32(meta + i * NAND_META_SIZE + 4, sequence);
        meta[i * NAND_META_SIZE + 9] = i + 1 == count ? last_type : META_LOG;
    }
    if ((uintptr_t)data % CACHEALIGN_SIZE)
    {
        memcpy(ftl_transfer, data, count * FTL_PAGE_BYTES);
        data = ftl_transfer;
    }
    rc = vfl_program_pages(vb * FTL_PAGES_PER_VB + off, count, data, meta);
    return rc ? ftl_write_error(rc) : 0;
}

/* Random-log consolidation and wear/refresh moves retain a fresh destination.
 * The final page is programmed alone after all preceding batches complete. */
static int ftl_consolidate(unsigned int lbn)
{
    struct ftl_log *log = ftl_find_log(lbn);
    int target, rc;
    uint32_t sequence;
    unsigned int off;
    if (data_sequence == UINT32_MAX)
        return ftl_write_error(NAND_ERR_FTL);
    target = ftl_allocate();
    if (target < 0)
        return target;
    sequence = ++data_sequence;
    for (off = 0; off < FTL_PAGES_PER_VB; )
    {
        uint32_t lpn = lbn * FTL_PAGES_PER_VB + off;
        unsigned int n = MIN(NAND_MAX_PAGES_PER_READ, FTL_PAGES_PER_VB - off);
        if (n > 1 && off + n == FTL_PAGES_PER_VB)
            n--; /* commit marker must not race another CE's unfinished page */
        rc = ftl_read_pages(lpn, n, ftl_transfer);
        if (rc)
            return ftl_write_error(rc);
        rc = ftl_data_pages(target, off, lpn, sequence, n,
                            off + n == FTL_PAGES_PER_VB ? META_DATA : META_LOG,
                            ftl_transfer);
        if (rc)
            return rc;
        off += n;
    }
    block_owner[ftl_map[lbn]] = BLOCK_AVAILABLE;
    ftl_map[lbn] = target;
    if (log)
    {
        unsigned int slot = log - ftl_logs;
        block_owner[log->vbn] = BLOCK_AVAILABLE;
        log->vbn = log->lbn = 0xffff;
        log->usn = UINT32_MAX;
        log_next[slot] = 0;
        ftl_reset_log(slot);
    }
    merge_count++;
    return 0;
}

static int ftl_open_log(unsigned int lbn)
{
    unsigned int slot, victim = 0;
    int vb, rc;
    for (slot = 0; slot < FTL_LOG_SLOTS; slot++)
    {
        if (ftl_logs[slot].vbn == 0xffff)
            break;
        if (ftl_logs[slot].usn < ftl_logs[victim].usn)
            victim = slot;
    }
    if (slot == FTL_LOG_SLOTS)
    {
        rc = ftl_consolidate(ftl_logs[victim].lbn);
        if (rc)
            return rc;
        slot = victim;
    }
    if (data_sequence == UINT32_MAX)
        return ftl_write_error(NAND_ERR_FTL);
    vb = ftl_allocate();
    if (vb < 0)
        return vb;
    ftl_logs[slot].vbn = vb;
    ftl_logs[slot].lbn = lbn;
    ftl_logs[slot].usn = ++data_sequence;
    log_next[slot] = 0;
    ftl_reset_log(slot);
    return slot;
}

static int ftl_write_pages(uint32_t sector, uint32_t count, const void *buffer)
{
    const uint8_t *data = buffer;
    int rc;
    if (!ftl_mounted || !ftl_writable || sector > ftl_pages || count > ftl_pages - sector
        || (count && !buffer))
        return NAND_ERR_FTL;
    if (!count)
        return 0;
    rc = ftl_begin_write();
    if (rc)
        return rc;
    while (count)
    {
        unsigned int lbn = sector / FTL_PAGES_PER_VB;
        struct ftl_log *log = ftl_find_log(lbn);
        int slot = log ? log - ftl_logs : -1;
        if (log && log_next[slot] == FTL_PAGES_PER_VB)
        {
            rc = ftl_consolidate(lbn);
            if (rc)
                return rc;
            log = NULL;
        }
        if (!log)
        {
            slot = ftl_open_log(lbn);
            if (slot < 0)
                return slot;
            log = &ftl_logs[slot];
        }
        unsigned int off = sector % FTL_PAGES_PER_VB;
        unsigned int n = MIN(count, MIN(NAND_MAX_PAGES_PER_READ,
                         MIN(FTL_PAGES_PER_VB - off, FTL_PAGES_PER_VB - log_next[slot])));
        _Bool complete = off + n == FTL_PAGES_PER_VB && log_next[slot] == off;
        for (unsigned int i = 0; complete && i < off; i++)
            complete = FTL_OFFS(slot, i) == i;
        if (complete && n > 1)
        {
            n--;
            complete = false;
        }
        rc = ftl_data_pages(log->vbn, log_next[slot], sector, log->usn, n,
                            complete ? META_DATA : META_LOG, data);
        if (rc)
            return rc;
        for (unsigned int i = 0; i < n; i++)
            FTL_OFFS(slot, off + i) = log_next[slot]++;
        if (complete)
        {
            /* Same on-flash completion rule as a consolidation: all pages
             * are in logical order, with the last carrying META_DATA. */
            block_owner[ftl_map[lbn]] = BLOCK_AVAILABLE;
            ftl_map[lbn] = log->vbn;
            log->vbn = log->lbn = 0xffff;
            log->usn = UINT32_MAX;
            log_next[slot] = 0;
            ftl_reset_log(slot);
            merge_count++;
        }
        sector += n;
        count -= n;
        data += n * FTL_PAGE_BYTES;
    }
    return 0;
}

static void ftl_count_reads(uint32_t lpn, unsigned int count)
{
    while (count--)
    {
        unsigned int vb = ftl_vpn(lpn++) / FTL_PAGES_PER_VB;
        if (read_count[vb] != UINT16_MAX)
            read_count[vb]++;
    }
}

/* Move at most one cold block per checkpoint. Dynamic allocation distributes
 * hot writes over free blocks; this step also brings cold blocks into that
 * pool. Read-disturb refresh takes priority over erase-count balancing. */
static int ftl_maintain(void)
{
    unsigned int lbn, cold = ftl_vbs, highest = 0;
    for (lbn = 0; lbn < vfl_usable; lbn++)
        highest = MAX(highest, erase_count[lbn]);
    for (lbn = 0; lbn < ftl_vbs; lbn++)
    {
        struct ftl_log *log = ftl_find_log(lbn);
        if (read_count[ftl_map[lbn]] >= FTL_READ_REFRESH
            || (log && read_count[log->vbn] >= FTL_READ_REFRESH))
            return ftl_consolidate(lbn);
        if (!log && (cold == ftl_vbs
                     || erase_count[ftl_map[lbn]] < erase_count[ftl_map[cold]]))
            cold = lbn;
    }
    if (merge_count >= 64 && cold != ftl_vbs
        && highest > erase_count[ftl_map[cold]] + FTL_WEAR_SPREAD)
    {
        merge_count = 0;
        return ftl_consolidate(cold);
    }
    return 0;
}

static int ftl_store_table(const void *table, unsigned int bytes,
                            unsigned int list, uint8_t type)
{
    unsigned int idx = 0;
    const uint8_t *p = table;
    while (bytes)
    {
        unsigned int n = MIN(bytes, FTL_PAGE_BYTES);
        int rc;
        memset(ftl_buf, 0xff, sizeof(ftl_buf));
        memcpy(ftl_buf, p, n);
        put32(ftl_cxt + list + idx * 4,
               vfl_ftl_ctrl[control_index] * FTL_PAGES_PER_VB + control_next);
        rc = ftl_control_page(type, idx, ftl_buf);
        if (rc)
            return rc;
        p += n;
        bytes -= n;
        idx++;
    }
    return 0;
}

int ftl_flush(void)
{
    unsigned int i, j, nfree = 0, nlogs = 0;
    unsigned int tables = 2 * ((vfl_usable * 2 + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES)
        + (ftl_vbs * 2 + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES
        + (FTL_LOG_SLOTS * FTL_PAGES_PER_VB * 2 + FTL_PAGE_BYTES - 1) / FTL_PAGE_BYTES;
    int rc;
    if (!ftl_mounted || write_failed)
        return NAND_ERR_FTL;
    if (!ftl_writable || !write_dirty)
        return 0;
    rc = ftl_maintain();
    if (rc)
        return rc;
    rc = ftl_control_space(tables + 2);
    if (rc || control_sequence < tables + 3)
        return ftl_write_error(rc ? rc : NAND_ERR_FTL);
    control_sequence -= tables + 2;

    memset(ftl_cxt, 0, sizeof(ftl_cxt));
    memset(ftl_cxt + 0x00e, 0xff, FTL_FREE_SLOTS * 2);
    for (i = 0; i < vfl_usable; i++)
        if (block_owner[i] != BLOCK_OWNED)
        {
            if (nfree == FTL_FREE_SLOTS)
                return ftl_write_error(NAND_ERR_FTL);
            if (block_owner[i] != BLOCK_ERASED)
            {
                rc = ftl_erase_block(i);
                if (rc)
                    return rc;
                block_owner[i] = BLOCK_ERASED;
            }
            put16(ftl_cxt + 0x00e + nfree++ * 2, i);
        }
    for (i = 0; i < FTL_RESTORE_SLOTS; i++)
    {
        uint8_t *entry = ftl_cxt + FTL_CXT_LOGS + i * FTL_CXT_LOG_SIZE;
        unsigned int current = 0;
        _Bool sequential = true;
        put32(entry, ftl_logs[i].usn);
        put16(entry + 4, ftl_logs[i].vbn);
        put16(entry + 6, ftl_logs[i].lbn);
        if (i == FTL_LOG_SLOTS || ftl_logs[i].vbn == 0xffff)
            continue;
        nlogs++;
        for (j = 0; j < FTL_PAGES_PER_VB; j++)
            if (FTL_OFFS(i, j) != 0xffff)
            {
                current++;
                if (FTL_OFFS(i, j) != j)
                    sequential = false;
            }
        put16(entry + 12, log_next[i]);
        put16(entry + 14, current);
        /* A log sealed on remount has no appendable tail, even when its
         * valid prefix is sequential. Other writers require equal used and
         * current counts before selecting their sequential merge path. */
        put32(entry + 16, sequential && current == log_next[i]);
    }
    if (nfree + nlogs != FTL_FREE_SLOTS)
        return ftl_write_error(NAND_ERR_FTL);
    put32(ftl_cxt, control_sequence);
    put32(ftl_cxt + 4, data_sequence);
    put16(ftl_cxt + 8, nfree);
    put16(ftl_cxt + 12, merge_count);
    for (i = 0; i < 3; i++)
        put16(ftl_cxt + 0x312 + i * 2, vfl_ftl_ctrl[i]);
    put32(ftl_cxt + 0x318, vfl_ftl_ctrl[control_index] * FTL_PAGES_PER_VB
                           + control_next + tables + 1);
    put32(ftl_cxt + 0x31c, 1);
    memset(ftl_cxt + 0x3b4, 0xff, 20);
    put32(ftl_cxt + 0x3d4, UINT32_MAX);
    put32(ftl_cxt + 0x7f8, 0x46560001);
    put32(ftl_cxt + 0x7fc, ~0x46560001ul);

    rc = ftl_store_table(erase_count, vfl_usable * 2, FTL_CXT_EC_PAGES, 'F');
    if (!rc)
        rc = ftl_store_table(read_count, vfl_usable * 2, FTL_CXT_RC_PAGES, 'I');
    if (!rc)
        rc = ftl_store_table(ftl_map, ftl_vbs * 2, FTL_CXT_MAP_PAGES, 'D');
    if (!rc)
        rc = ftl_store_table(ftl_offsets, FTL_LOG_SLOTS * FTL_PAGES_PER_VB * 2,
                             FTL_CXT_OFF_PAGES, 'E');
    if (rc)
        return rc;
    put32(ftl_cxt + FTL_CXT_STATS_PAGE,
           vfl_ftl_ctrl[control_index] * FTL_PAGES_PER_VB + control_next);
    put32(write_stats + FTL_PAGE_BYTES - 4, 0x10002);
    rc = ftl_control_page('G', 0, write_stats);
    if (rc)
        return rc;
    memset(ftl_buf, 0xff, sizeof(ftl_buf));
    memcpy(ftl_buf, ftl_cxt, sizeof(ftl_cxt));
    rc = ftl_control_page('C', UINT32_MAX, ftl_buf);
    if (!rc)
        write_dirty = false;
    return rc;
}

/* fw_disk 0x080339a8 / 0x08034694: a pending VFL geometry change requires
 * a fresh FTL, not recovery using the old block map. Keep the pending flag
 * durable until the new checkpoint has been stored successfully. */
static int ftl_format_pending(void)
{
    unsigned int i, bank;
    uint32_t generation = 0;
    int rc;

    memset(ftl_logs, 0xff, sizeof(ftl_logs));
    memset(ftl_offsets, 0xff, sizeof(ftl_offsets));
    memset(ftl_blank, 1, sizeof(ftl_blank));
    for (i = 0; i < ftl_vbs; i++)
        ftl_map[i] = i + FTL_EXTRA_VBS;
    for (i = 0; i < 3; i++)
        vfl_ftl_ctrl[i] = i;
    /* Start from empty counters/cursors without interpreting old checkpoints. */
    memset(block_owner, BLOCK_ERASED, sizeof(block_owner));
    memset(erase_count, 0, sizeof(erase_count));
    memset(read_count, 0, sizeof(read_count));
    memset(write_stats, 0, sizeof(write_stats));
    memset(log_next, 0, sizeof(log_next));
    data_sequence = 0;
    control_sequence = UINT32_MAX;
    control_next = FTL_PAGES_PER_VB;
    control_index = allocation_cursor = merge_count = 0;
    write_failed = false;
    write_dirty = true;
    for (i = 0; i < vfl_usable; i++)
    {
        rc = ftl_erase_block(i);
        if (rc)
            return rc;
        if (i < 3 || i >= FTL_EXTRA_VBS)
            block_owner[i] = BLOCK_OWNED;
    }
    for (bank = 0; bank < FTL_BANKS; bank++)
        if (get32(vfl_cxt[bank] + VFL_CXT_USN) > generation)
            generation = get32(vfl_cxt[bank] + VFL_CXT_USN);
    rc = vfl_store_context(generation % FTL_BANKS);
    if (rc)
        return rc;
    ftl_mounted = ftl_writable = true;
    rc = ftl_flush();
    if (!rc)
    {
        for (bank = 0; bank < FTL_BANKS; bank++)
        {
            put32(vfl_cxt[bank] + VFL_CXT_FTL_TYPE, 1);
            memset(vfl_cxt[bank] + 0x6ca, 0, 16);
            vfl_update_checksum(vfl_cxt[bank]);
        }
        rc = vfl_store_context((generation + 1) % FTL_BANKS);
    }
    if (rc)
        ftl_mounted = ftl_writable = false;
    return rc;
}
