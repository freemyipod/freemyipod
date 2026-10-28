/***************************************************************************
 * Copyright (C) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Shared nano4g/nano5g mutation and raw-transfer implementation. Included by
 * the physical driver after its controller, timing and reset helpers. Caller
 * holds the storage lock; request buffers are shared with the read path.
 ****************************************************************************/
#ifndef S5L_NAND_WRITE_H
#define S5L_NAND_WRITE_H
#include "nand-plan.h"
#include "nand-raw.h"

static uint8_t nand_failed_ce;

unsigned int nand_get_last_failure_mask(void)
{
    return nand_failed_ce;
}

static uint32_t nand_commands[2 * NAND_MAX_PAGES_PER_READ + 1] CACHEALIGN_ATTR;

/* Nano5g-only hole/stride handling is excluded from nano4g builds. Nano4g
 * still needs multiple-die command/status support; its supplied row map is dense. */
static _Bool nand_valid_row(uint32_t row)
{
    unsigned int block = row / nand_geom.page_stride;
    return row % nand_geom.page_stride < nand_geom.pages_per_block &&
           block / nand_geom.die_stride < nand_geom.dies &&
           block % nand_geom.die_stride < nand_geom.blocks_per_die;
}

_Bool nand_write_supported(void)
{
    return nand_phys_ready;
}

static int nand_finish_mutation(void)
{
    int rc = fmss_start();
    if (!rc)
        rc = fmss_wait();
    uint32_t result = FMSS_RD(FMSS_RESULT);
    nand_failed_ce = FMSS_RD(0xd04);
    /* Device, nano 5G: the program's final status poll ends ~10 us after an
     * erase confirm while the chip is still erasing, and a read sent then
     * returns zeros. Its FAIL bit is read before the operation ends too.
     * Wait for R/B on every CE the run used (d50), as the CPU-side ready
     * wait (70h, status read until R/B) does, then take each chip's FAIL
     * bit from its status byte. */
    if (!rc)
    {
        unsigned int used = FMSS_RD(0xd50) & 0xff;
        if (nand_wait_ready(used))
            rc = NAND_ERR_TIMEOUT;
        for (unsigned int ce = 0; !rc && ce < 8; ce++)
        {
            if (!(used & (1u << ce)))
                continue;
            int status = nand_read_status(ce);
            if (status < 0)
                rc = NAND_ERR_TIMEOUT;
            else if (status & NSTAT_FAIL)
            {
                nand_failed_ce |= 1u << ce;
                result |= 1u << 28;
            }
        }
    }
    if (rc || (result & (1u << 27)))
        rc = NAND_ERR_TIMEOUT;
    else if (result & (1u << 28))
        rc = NAND_ERR_WRITE;
    if (rc)
        nand_reset_chips();
    return rc;
}

/* All descriptor lists are built and validated before this function starts
 * the engine. Boot pages use three BCH16 sectors without metadata. */
static int nand_execute_plan(const struct nand_program_plan *plan,
                             const void *data, _Bool boot, _Bool erase)
{
    unsigned int bytes = boot ? 1536 : nand_geom.page_size;
    unsigned int chunks = (bytes + 2047) / 2048;
    clean_dcache_range((uint32_t)nand_commands, (2 * plan->count + 1) * sizeof(uint32_t));
    if (!erase)
    {
        clean_dcache_range((uint32_t)data, plan->count * bytes);
        clean_dcache_range((uint32_t)nand_dmalist, plan->count * chunks * sizeof(uint32_t));
        clean_dcache_range((uint32_t)nand_meta, plan->count * NAND_META_SIZE);
    }
    if (boot)
        FMSS_WR(0x808, 0x80);
    FMSS_WR(P_ROWS, NAND_DMA_ADDR(nand_commands));
    FMSS_WR(P_META, boot || erase ? 0 : NAND_DMA_ADDR(nand_meta));
    FMSS_WR(P_DMALIST, NAND_DMA_ADDR(nand_dmalist));
    FMSS_WR(0xd7c, bytes / 512);
    FMSS_WR(0xd78, plan->status_row);
    /* The OS's ERASE program selects with the read timing (P_TIMING) */
    FMSS_WR(0xd48, boot ? 0x77000 : erase ? nand_timing : nand_program_timing);
    FMSS_WR(P_TIMING, boot ? 0x77000 : nand_timing);
    FMSS_WR(P_ECCMODE, boot ? 6 : (nand_ecc_mode & ~1u) | 2);
    FMSS_WR(P_ECCCFG, boot ? 6 : (nand_ecc_cfg & ~1u) | 2);
    FMSS_WR(P_HDRLEN, boot ? 25 : nand_ecc_hdrlen);
    FMSS_WR(P_PARLEN, boot ? 25 : nand_ecc_parlen);
    FMSS_WR(FMSS_PROG, NAND_DMA_ADDR(fmss_prog_program));
    return nand_finish_mutation();
}

static struct nand_program_plan nand_new_plan(void)
{
    struct nand_program_plan plan = {
        nand_commands, nand_dmalist, nand_meta, NAND_MAX_PAGES_PER_READ, 0, 0
    };
    return plan;
}

int nand_program_pages(unsigned int count, const uint8_t *banks,
                       const uint32_t *rows, const void *data, const void *meta)
{
    if (!nand_phys_ready)
        return NAND_ERR_NOCHIP;
    if (!data || !meta || !banks || !rows || !count ||
        count > NAND_MAX_PAGES_PER_READ || (uintptr_t)data % CACHEALIGN_SIZE)
        return NAND_ERR_UNSUPPORTED;
    for (unsigned int i = 0; i < count; i++)
        if (banks[i] >= nand_geom.banks || !nand_valid_row(rows[i]))
            return NAND_ERR_UNSUPPORTED;
    struct nand_program_plan plan = nand_new_plan();
    if (!nand_plan_scattered(&plan, nand_geom.ce, nand_geom.banks,
                             nand_geom.page_size, count, banks, rows,
                             NAND_DMA_ADDR(data), meta))
        return NAND_ERR_UNSUPPORTED;
    return nand_execute_plan(&plan, data, false, false);
}

/* Data and metadata are plane-major within each bank/plane stripe. Address
 * alignment and correct-bank flags have the same meaning as OSOS's sequential
 * operation: both must hold before the selected multi-plane protocol is used. */
int nand_program_sequential(unsigned int count, unsigned int planes,
                            const uint32_t *row_base, const void *data,
                            const void *meta, _Bool aligned, _Bool correct_bank)
{
    if (!nand_phys_ready)
        return NAND_ERR_NOCHIP;
    unsigned int stripe = nand_geom.banks * planes;
    if (!data || !meta || !row_base || planes == 0 || planes > 2 || !count ||
        count > NAND_MAX_PAGES_PER_READ || !stripe || count % stripe ||
        (uintptr_t)data % CACHEALIGN_SIZE)
        return NAND_ERR_UNSUPPORTED;
    /* Validate every row before scheduling anything, including arithmetic
     * overflow and die holes. The planner's layout selects the NAND commands. */
    for (unsigned int i = 0; i < stripe; i++)
        for (unsigned int step = 0; step < count / stripe; step++)
            if (row_base[i] > UINT32_MAX - step || !nand_valid_row(row_base[i] + step))
                return NAND_ERR_UNSUPPORTED;
    struct nand_program_plan plan = nand_new_plan();
    if (!nand_plan_sequential(&plan, nand_geom.ce, nand_geom.banks, planes,
                              nand_geom.page_size, nand_geom.blocks,
                              nand_geom.pages_per_block, nand_geom.layout,
                              count, row_base, NAND_DMA_ADDR(data), meta,
                              aligned, correct_bank))
        return NAND_ERR_UNSUPPORTED;
    return nand_execute_plan(&plan, data, false, false);
}

int nand_device_program(unsigned int bank, uint32_t page,
                        const void *data, const void *meta)
{
    if (!nand_phys_ready || bank >= nand_geom.banks)
        return NAND_ERR_NOCHIP;
    if (!data || !nand_valid_row(page))
        return NAND_ERR_UNSUPPORTED;
    if ((uintptr_t)data % CACHEALIGN_SIZE)
    {
        memcpy(nand_bounce, data, nand_geom.page_size);
        data = nand_bounce;
    }
    uint8_t filler[NAND_META_SIZE];
    if (!meta)
    {
        memset(filler, 0xa5, sizeof(filler));
        meta = filler;
    }
    uint8_t selected = bank;
    return nand_program_pages(1, &selected, &page, data, meta);
}

int nand_erase_blocks(unsigned int count, const uint8_t *banks,
                      const uint32_t *blocks)
{
    if (!nand_phys_ready)
        return NAND_ERR_NOCHIP;
    if (!count || count > NAND_MAX_PAGES_PER_READ || !banks || !blocks)
        return NAND_ERR_UNSUPPORTED;
    struct nand_program_plan plan = nand_new_plan();
    uint8_t busy = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        if (banks[i] >= nand_geom.banks || blocks[i] >= nand_geom.blocks)
            return NAND_ERR_UNSUPPORTED;
        uint8_t ce = 1u << nand_geom.ce[banks[i]];
        nand_commands[2 * i] = ((busy & ce) ? 0x70000000u : 0) | 0x0060d000 | ce;
/* Nano5g alone maps dense block indices around row-address holes. */
        nand_commands[2 * i + 1] = nand_block_row(blocks[i]);
        busy |= ce;
    }
    nand_commands[2 * count] = 0;
    plan.count = count;
    /* One ERASE run per block in the OS's order (fw_disk
     * m2fmssEraseSingleBlock 0x080548fc): read timing, 60h + three row
     * cycles + D0h, status; then the ready wait and FAIL check above. */
    (void)plan;
    for (unsigned int i = 0; i < count; i++)
    {
        uint8_t ce = 1u << nand_geom.ce[banks[i]];
        int rc;
        FMSS_WR(FMSS_PROG, NAND_DMA_ADDR(fmss_prog_erase));
        FMSS_WR(P_ROWS, nand_block_row(blocks[i]));
        FMSS_WR(P_CEMASKS, ce);
        FMSS_WR(P_COUNT, 1);
        FMSS_WR(P_TIMING, nand_timing);
        FMSS_WR(0xd04, 0);
        FMSS_WR(0xd50, ce);         /* CEs used, for nand_finish_mutation */
        rc = nand_finish_mutation();
        if (rc)
            return rc;
    }
    return NAND_OK;
}

int nand_device_erase(unsigned int bank, uint32_t block)
{
    if (!nand_phys_ready || bank >= nand_geom.banks)
        return NAND_ERR_NOCHIP;
    uint8_t selected = bank;
    return nand_erase_blocks(1, &selected, &block);
}

int nand_program_boot_page(unsigned int bank, uint32_t row, const void *data)
{
    if (!nand_phys_ready || bank >= nand_geom.banks)
        return NAND_ERR_NOCHIP;
    if (!data || !nand_valid_row(row))
        return NAND_ERR_UNSUPPORTED;
    memcpy(nand_bounce, data, 1536);
    struct nand_program_plan plan = nand_new_plan();
    plan.count = 1;
    nand_commands[0] = 0x00801000 | (1u << nand_geom.ce[bank]);
    nand_commands[1] = row;
    nand_commands[2] = 0;
    nand_dmalist[0] = NAND_DMA_ADDR(nand_bounce);
    return nand_execute_plan(&plan, nand_bounce, true, false);
}

int nand_read_boot_page(unsigned int bank, uint32_t row, void *data)
{
    if (!nand_phys_ready || bank >= nand_geom.banks)
        return NAND_ERR_NOCHIP;
    if (!data || !nand_valid_row(row))
        return NAND_ERR_UNSUPPORTED;
    invalidate_dcache_range((uint32_t)nand_bounce, 1536);
    FMSS_WR(P_ROWS, row);
    FMSS_WR(P_CEMASKS, 1u << nand_geom.ce[bank]);
    FMSS_WR(P_DMALIST, NAND_DMA_ADDR(nand_bounce));
    FMSS_WR(FMSS_PROG, NAND_DMA_ADDR(fmss_prog_bootread));
    int rc = fmss_start();
    if (!rc)
        rc = fmss_wait();
    uint32_t result = FMSS_RD(FMSS_RESULT);
    if (rc || (result & (1u << 27)))
    {
        nand_reset_chips();
        return NAND_ERR_TIMEOUT;
    }
    invalidate_dcache_range((uint32_t)nand_bounce, 1536);
    memcpy(data, nand_bounce, 1536);
    if (result & FMSS_RESULT_UECC)
        return NAND_ERR_ECC;
    return (result & FMSS_RESULT_ERASED) && !(result & 0x1ffff)
           ? NAND_ERASED : NAND_OK;
}

static uint32_t nand_raw_mmio_read(void *context, unsigned int offset)
{
    (void)context;
    return FMSS_RD(offset);
}

static void nand_raw_mmio_write(void *context, unsigned int offset, uint32_t value)
{
    (void)context;
    FMSS_WR(offset, value);
}

static int nand_raw_mmio_wait(void *context, uint32_t mask)
{
    (void)context;
    uint32_t start = USEC_TIMER;
    while (!(FMSS_RD(0x048) & mask))
        if (nand_elapsed(start, PROG_TIMEOUT_US))
            return NAND_ERR_TIMEOUT;
    FMSS_WR(0x048, mask);
    return NAND_OK;
}

int nand_transfer_raw(unsigned int bank, uint32_t row, void *data,
                      void *spare, _Bool write)
{
    if (!nand_phys_ready || bank >= nand_geom.banks)
        return NAND_ERR_NOCHIP;
    if (!data || !spare || !nand_valid_row(row))
        return NAND_ERR_UNSUPPORTED;
    const struct nand_raw_bus bus = {
        NULL, nand_raw_mmio_read, nand_raw_mmio_write, nand_raw_mmio_wait
    };
    int rc = nand_raw_transfer(&bus, nand_geom.ce[bank],
                              write ? nand_program_timing : nand_timing,
                              row, data, nand_geom.page_size,
                              spare, nand_geom.spare_size, write);
    FMSS_WR(0, FMSS_RD(0) & 0xfcfffe01u);
    if (rc)
        nand_reset_chips();
    return rc;
}
#endif
