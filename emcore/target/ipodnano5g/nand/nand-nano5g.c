/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Michael Sparmann
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

/* S5L8730 NAND driver. The FMC sequencer issues commands, decodes
 * BCH and transfers page data by DMA. Static buffers require caller locking.
 * DMA buffers must be cache-line aligned; unaligned reads use nand_bounce.
 *
 * This is the driver of the nano 4G with the differences of the nano 5G:
 * 4096- or 8192-byte pages (16 ECC sectors), parts with two dies per chip
 * enable, its own part and population tables, a bus timing calculation with
 * eight values per population, and no reset program for the sequencer: the
 * CPU resets the chips through the FMC registers itself. */

#include "global.h"
#include "../s5l8730.h"
#include "timer.h"
#include "mmu.h"
#include "clockgates.h"
#include "nand-target.h"
#include "ftl-target.h"
#include "fmss-nano5g.h"

/* emCORE maps DRAM and IRAM flat: a pointer is a DMA bus address. */
#define FMSS_RD(off)        (*(volatile uint32_t *)(FMC_BASE + (off)))
#define FMSS_WR(off, val)   (*(volatile uint32_t *)(FMC_BASE + (off)) = (val))
#define NAND_DMA_ADDR(p)    ((uint32_t)(uintptr_t)(p))

/* ==== Registers ==== */
/* FMC / FMSS registers, offsets from FMC_BASE */
#define FMC_CTRL0             0x000   /* chip enables, bus timing, mode */
#define FMC_CTRL0_CPU       0x77001  /* CPU-driven commands: slowest timing, no DMA */
#define FMC_CTRL0_CE(mask)  ((uint32_t)(mask) << 1)  /* chip enable n = bit n + 1 */
#define FMC_CTRL1             0x004   /* transfer engine control */
#define FMCTRL1_RESET       0x800  /* transfer engine reset request (_resetFMSS) */
#define FMCTRL1_IDLE        (1u << 30)  /* reads 1 when the FMC is idle after the reset */
#define FMCTRL1_RELEASE     0x80000000u  /* written after the reset completes */
#define FMCTRL1_STATUS_READ 0xea   /* poll the NAND status byte until ready */
#define FMCTRL1_STATUS_END  0x20   /* ends the status poll */
#define FMCTRL1_STATUS_LATCH 0xca  /* latch the NAND status byte into FMC_NSTAT */
#define FMCTRL1_LATCH_END   0x30020  /* after a latched status read (FMSS programs) */
#define FMC_CMD               0x008   /* NAND command byte, sent on write */
#define FMC_DATA_SEL          0x014   /* data path select */
#define FMC_STATUS            0x048   /* write 1 to clear */
#define FMC_NSTAT             0x04c   /* latched NAND status byte */
#define NSTAT_READY         0x40   /* I/O6: ready */
#define NSTAT_FAIL          0x01   /* I/O0: the last program/erase failed */
#define FMC_STATUS_CMD      (1u << 1)   /* command sent */
#define FMC_STATUS_READY    (1u << 23)  /* chip ready (after a status poll) */
#define FMC_STATUS_READY_CLR 0x1800000  /* clears the ready bits before a poll */
#define FMC_STATUS_CLEAR_ALL 0x0ff00ffe /* clears the transfer status */
#define ECC_RESET           0x81c  /* ECC unit reset, write 1 (end of _resetFMSS) */
#define FMSS_CTRL           0xc00  /* sequencer control (CPU only) */
/* The OS that writes (fw_disk _startFMSS 0x08043268) starts with bit 6 set.
 * Device, Oct 25: with the bootloader's 0xffb5 every program and erase is
 * ignored (status e0, ready at once), CPU-driven commands included; with
 * 0xfff5 enables the program/erase path. */
#define FMSS_CTRL_START     0xfff5  /* sequencer start command */
#define FMSS_CTRL_RESUME    0x1  /* OR-ed in to continue after a pause */
#define FMSS_CTRL_ABORT     0x2  /* abort a running program (STATUS 4) */
#define FMSS_CTRL_STOP      0x8  /* stop request, first write of _resetFMSS */
#define FMSS_CTRL_FMCRESET  0x80000  /* written before the FMC reset */
#define FMSS_PROG           0xc04   /* bus address of the program */
#define FMSS_STATUS         0xc08   /* 0 idle, 3 aborted, 4 busy */
#define FMSS_IRQ            0xc0c   /* write 1 to clear */
#define FMSS_IRQ_DONE       (1u << 0)  /* program reached END */
#define FMSS_IRQ_FAULT      (1u << 2)  /* a WAIT timed out, fault handler ran */
#define FMSS_IRQ_PAUSE      (1u << 3)  /* program paused (stepped over by the CPU) */
#define FMSS_IRQ_ALL        0xff  /* clear-all value of _startFMSS */
#define FMSS_IRQ_MASK       (FMSS_IRQ_DONE | FMSS_IRQ_FAULT | FMSS_IRQ_PAUSE)
#define FMSS_RESULT         0xc30   /* sequencer r6 when the program ended */
#define FMSS_RESULT_UECC    (1u << 30)
#define FMSS_RESULT_ERASED  (1u << 29)
#define FMSS_RESULT_NCORR   0x1ffff  /* bit n: some sector had n bits corrected */
#define FMSS_PC             0xc38  /* sequencer program counter (byte offset) */
#define FMSS_BUFRST         0xc60  /* buffer reset request: 1 = request, 0 = release */
#define FMSS_BUFRST_OK      0xc64  /* becomes 1 when the buffer reset is done */
#define FMSS_BUFCFG         0xc6c  /* buffer config: 0 = handshake path */
#define FMSS_BUFCFG_BYPASS  0x10  /* _resetFMSS skips the handshake if set */

/* The CPU writes these before every run; the READ program advances the list
 * pointers (P_ROWS, P_CEMASKS, P_DMALIST, P_META, P_ECCBUF) in place, so they
 * are always rewritten. Scratch words 0xd3c.. belong to the
 * microcode (V_D3C = page ECC summary, 0xd40 = groups of four sectors left). */
/* Parameter words the programs read */
#define P_IDBUF             0xd08  /* READ ID: bus address of u32 out[CEs] */
#define P_ROWS              0xd0c   /* READ: u32 row list; READ ID: CE count */
#define P_CEMASKS           0xd10  /* READ: u32 list, 1 << CE per page */
#define P_ECCBUF            0xd14  /* READ: u8 status per sector out, 0 = none */
#define P_COUNT             0xd18  /* READ: number of pages */
#define P_META              0xd1c  /* READ: 12 meta bytes per page out */
#define P_DMALIST           0xd20  /* READ: u32 bus address per 2 KiB chunk */
#define P_STATS             0xd24  /* READ: u32[18] ECC histogram, 0 = none */
#define P_CHUNKS            0xd28  /* DMA chunks per page (sectors / 4) */
#define P_ECCMODE           0xd2c  /* ECC_CTRL value for sectors 1..n */
#define P_ECCCFG            0xd30  /* ECC_CTRL value for sector 0 (+ meta) */
#define P_HDRLEN            0xd34  /* meta + parity bytes of sector 0, - 1 */
#define P_PARLEN            0xd38  /* parity bytes of sectors 1..n, - 1 */
#define P_TIMING            0xd4c  /* FMC_CTRL0 timing fields (nand_calc_timing) */

/* FMC and ECC clock gates: PWRCON0 bits 5 and 9. */
#define CLOCKGATE_FMC       5
#define CLOCKGATE_FMC_ECC   9

/* 100 us per register poll, 2 s per program, 20 ms for a CPU-driven command,
 * 10 ms for the chips to settle after a reset. */
#define POLL_TIMEOUT_US     100     /* register handshakes */
#define PROG_TIMEOUT_US     2000000 /* one program run */
#define FMC_TIMEOUT_US      20000   /* CPU-driven command or status poll */
#define RESET_SETTLE_US     10000   /* after the reset of all chips */

/* The bus clock the FMC timing is computed for. The first-stage loader and
 * Rockbox both run HCLK at 137 MHz on the nano 5G (PLL0 274.67 MHz, CLKCON1
 * 0x404040) and Rockbox never changes the HCLK divider on this SoC. A slower
 * real clock only makes the timings longer. */
#define NAND_BUS_MHZ        137

/* At most 16 ECC sectors of 512 bytes per page; one DMA entry per 2048 bytes. */
#define MAX_SECTORS_PER_PAGE (NAND_PAGE_SIZE / 512)
#define MAX_CHUNKS_PER_PAGE  (NAND_PAGE_SIZE / 2048)  /* DMA list entries */

#include "nand-chip-table.h"

/* ==== Driver state and DMA buffers ==== */
static struct nand_geometry nand_geom;
static uint32_t nand_timing; /* FMC_CTRL0 read timing fields */
static uint32_t nand_program_timing;
static uint32_t nand_ecc_mode, nand_ecc_cfg, nand_ecc_hdrlen, nand_ecc_parlen;  /* P_ECCMODE..P_PARLEN values */
static uint32_t nand_pages_read;  /* diagnostics: pages handed to the sequencer */
static _Bool nand_phys_ready;  /* nand_phys_init() succeeded */

/* Buffers the sequencer reads or writes by DMA: cache-line aligned, and the
 * request lists are cleaned from the D-cache before every run */
static uint32_t nand_idbuf[NAND_MAX_BANKS] CACHEALIGN_ATTR;  /* READ ID output, one u32 per CE */
static uint32_t nand_rows[NAND_MAX_PAGES_PER_READ] CACHEALIGN_ATTR;  /* row per page (P_ROWS) */
static uint32_t nand_cemasks[NAND_MAX_PAGES_PER_READ] CACHEALIGN_ATTR;  /* 1 << CE per page (P_CEMASKS) */
static uint32_t nand_dmalist[NAND_MAX_PAGES_PER_READ * MAX_CHUNKS_PER_PAGE]  /* up to 4 bus addresses per page */
    CACHEALIGN_ATTR;
static uint8_t nand_meta[NAND_MAX_PAGES_PER_READ * NAND_META_SIZE]  /* 12 meta bytes per page (P_META) */
    CACHEALIGN_ATTR;
static uint8_t nand_secstat[MAX_SECTORS_PER_PAGE] CACHEALIGN_ATTR;  /* status per sector, single-page reads */
static uint32_t nand_hist[18] CACHEALIGN_ATTR;  /* per-run ECC histogram */
static uint8_t nand_bounce[NAND_PAGE_SIZE] CACHEALIGN_ATTR;  /* for unaligned caller buffers */

/* ==== Sequencer control ==== */

#include "nand-controller.h"

/* ==== Chip reset and detect ==== */

/* Wait for an FMC status bit, then acknowledge it */
static int fmc_wait_status(uint32_t bit)
{
    uint32_t start = USEC_TIMER;

    while (!(FMSS_RD(FMC_STATUS) & bit))
        if (nand_elapsed(start, FMC_TIMEOUT_US))
            return NAND_ERR_TIMEOUT;
    FMSS_WR(FMC_STATUS, bit);
    return 0;
}

/* Reset the chips on the chip enables in ce_mask (bit n = CE n) with the
 * RESET command (0xff), sent by the CPU. */
static int nand_cpu_reset(unsigned int ce_mask)
{
    int rc;

    FMSS_WR(FMC_CTRL0, FMC_CTRL0_CPU | FMC_CTRL0_CE(ce_mask));
    FMSS_WR(FMC_CMD, 0xff);
    rc = fmc_wait_status(FMC_STATUS_CMD);
    FMSS_WR(FMC_CTRL0, FMC_CTRL0_CPU);
    return rc;
}

/* Wait until every chip in ce_mask reports ready: READ STATUS (0x70), then
 * the controller polls the status byte of one chip enable at a time. */
static int nand_wait_ready(unsigned int ce_mask)
{
    unsigned int ce;
    int rc = 0;

    FMSS_WR(FMC_CTRL0, FMC_CTRL0_CPU | FMC_CTRL0_CE(ce_mask));
    FMSS_WR(FMC_CMD, 0x70);
    if (fmc_wait_status(FMC_STATUS_CMD))
        rc = NAND_ERR_TIMEOUT;
    for (ce = 0; ce < NAND_MAX_BANKS && !rc; ce++)
    {
        if (!(ce_mask & (1u << ce)))
            continue;
        FMSS_WR(FMC_CTRL0, FMC_CTRL0_CPU | FMC_CTRL0_CE(1u << ce));
        FMSS_WR(FMC_STATUS, FMC_STATUS_READY_CLR);
        FMSS_WR(FMC_DATA_SEL, 1);
        FMSS_WR(FMC_CTRL1, FMCTRL1_STATUS_READ);
        rc = fmc_wait_status(FMC_STATUS_READY);
        FMSS_WR(FMC_CTRL1, FMCTRL1_STATUS_END);
        FMSS_WR(FMC_STATUS, FMC_STATUS_CLEAR_ALL);
    }
    FMSS_WR(FMC_CTRL0, FMC_CTRL0_CPU);
    return rc;
}

/* The status byte of the chip on CE ce once it is ready: READ STATUS (0x70),
 * latch the byte (FMCTRL1 0xca) and poll FMC_NSTAT for ready, the sequence
 * the FMSS programs use before they test the FAIL bit. Returns the byte, or
 * -1 on a timeout. */
static int nand_read_status(unsigned int ce)
{
    uint32_t start, status;
    int rc;

    FMSS_WR(FMC_CTRL0, FMC_CTRL0_CPU | FMC_CTRL0_CE(1u << ce));
    FMSS_WR(FMC_CMD, 0x70);
    rc = fmc_wait_status(FMC_STATUS_CMD) ? -1 : 0;
    if (!rc)
    {
        FMSS_WR(FMC_CTRL1, FMCTRL1_STATUS_LATCH);
        start = USEC_TIMER;
        while (!((status = FMSS_RD(FMC_NSTAT)) & NSTAT_READY))
            if (nand_elapsed(start, FMC_TIMEOUT_US))
            {
                rc = -1;
                break;
            }
        FMSS_WR(FMC_CTRL1, FMCTRL1_LATCH_END);
        FMSS_WR(FMC_STATUS, FMC_STATUS_READY_CLR);
        if (!rc)
            rc = status & 0xff;
    }
    FMSS_WR(FMC_CTRL0, FMC_CTRL0_CPU);
    return rc;
}

/* Diagnostics: the status byte of bank's chip (bit 7 = not write
 * protected, bit 6 = ready, bit 0 = last program/erase failed), or -1. */
int nand_read_chip_status(unsigned int bank)
{
    if (!nand_phys_ready || bank >= nand_geom.banks)
        return -1;
    return nand_read_status(nand_geom.ce[bank]);
}

/* Reset the chips on ce_mask and read four ID bytes from each of CE0..CE7
 * with the READ ID program (P_ROWS = 8 CEs). nand_idbuf[ce] = ID bytes 0..3
 * little endian; an empty CE reads 0 or 0xffffffff. The first detection
 * resets all eight chip enables and waits a fixed time; later ones wait for
 * the chips in use to become ready. The ID buffer is invalidated before the
 * run (the sequencer writes it by DMA) and again afterwards. */
static int nand_reset_and_read_ids(unsigned int ce_mask)
{
    if (fmss_reset() || nand_cpu_reset(ce_mask))
        return NAND_ERR_TIMEOUT;
    if (ce_mask == 0xff)
    {
        udelay(RESET_SETTLE_US);
    }
    else if (nand_wait_ready(ce_mask))
        return NAND_ERR_TIMEOUT;

    FMSS_WR(FMSS_PROG, NAND_DMA_ADDR(fmss_prog_readid));
    FMSS_WR(P_IDBUF, NAND_DMA_ADDR(nand_idbuf));
    FMSS_WR(P_ROWS, NAND_MAX_BANKS);
    if (fmss_clear_irq())
        return NAND_ERR_TIMEOUT;
    invalidate_dcache_range((uint32_t)nand_idbuf, sizeof(nand_idbuf));
    if (fmss_start() || fmss_wait())
        return NAND_ERR_TIMEOUT;
    invalidate_dcache_range((uint32_t)nand_idbuf, sizeof(nand_idbuf));
    return 0;
}

/* After a failed read, reset the chips and verify their IDs. */

static int nand_reset_chips(void)
{
    unsigned int i, mask = 0;

    for (i = 0; i < nand_geom.banks; i++)
        mask |= 1u << nand_geom.ce[i];
    if (nand_reset_and_read_ids(mask))
        return NAND_ERR_TIMEOUT;
    for (i = 0; i < nand_geom.banks; i++)
        if (nand_idbuf[nand_geom.ce[i]] != nand_geom.id)
            return NAND_ERR_NOCHIP;
    return 0;
}

/* Decode ID fields for diagnostics using the legacy Samsung/Hynix format.
 * Other vendors encode spare size differently; use the part table for geometry. */
void nand_decode_id(uint32_t id, struct nand_id *out)
{
    static const struct { uint8_t code; const char *name; } makers[] =
    {
        { 0xad, "Hynix" }, { 0x98, "Toshiba" }, { 0xec, "Samsung" },
        { 0x2c, "Micron" }, { 0x89, "Intel" },
    };
    uint8_t b3 = (id >> 16) & 0xff, b4 = id >> 24;
    unsigned int i;

    out->maker = id & 0xff;
    out->device = (id >> 8) & 0xff;
    out->byte3 = b3;
    out->byte4 = b4;
    out->maker_name = "unknown";
    for (i = 0; i < ARRAYLEN(makers); i++)
        if (makers[i].code == out->maker)
            out->maker_name = makers[i].name;
    out->dies_per_ce = 1 << (b3 & 3);  /* byte 3 bits 1..0 */
    out->cell_levels = 2 << ((b3 >> 2) & 3);  /* bits 3..2: 2 = SLC, 4 = MLC */
    out->planes = 1 << ((b3 >> 4) & 3);  /* bits 5..4 */
    out->interleave = (b3 >> 6) & 1;  /* bit 6 */
    out->cache_program = (b3 >> 7) & 1;  /* bit 7 */
    out->page_size = 1024 << (b4 & 3);  /* byte 4 bits 1..0: 1/2/4/8 KiB */
    out->spare_per_512 = (b4 & 4) ? 16 : 8;  /* bit 2 */
    out->block_kib = 64 << ((b4 >> 4) & 3);  /* bits 5..4: 64..512 KiB */
    out->bus_width = (b4 & 0x40) ? 16 : 8;  /* bit 6 */
}

static struct nand_id nand_id_info;
static _Bool nand_id_valid;   /* some chip enable answered */
static _Bool nand_ids_valid;  /* a detection ran: nand_idbuf holds READ ID */

const struct nand_id *nand_get_id(void)
{
    return nand_id_valid ? &nand_id_info : NULL;
}

/* ==== Timing ==== */

/* Clocks of period ns_clk needed for x ns, minus one, at least 0 */
static unsigned int clocks_minus1(unsigned int x, unsigned int ns_clk)
{
    unsigned int c = (x + ns_clk - 1) / ns_clk;

    return c ? c - 1 : 0;
}

/* Convert the eight timing values of a population (ns) to the FMC_CTRL0
 * read timing fields: setup clocks in [18:16], hold clocks in [14:12] and
 * the data valid (EDO) clocks in [30:28], each at most 7. Uses the truncated
 * bus-clock period. Returns false if a value does not fit. */
static _Bool nand_calc_timing(const uint8_t *t, uint32_t *timing)
{
    unsigned int ns = 1000 / NAND_BUS_MHZ;     /* per clock, rounded down */
    unsigned int setup = clocks_minus1(t[4] + 4, ns);
    unsigned int valid = ns * (setup + 1);      /* time until data is valid */
    unsigned int need = MAX((unsigned int)t[6] + 6, (unsigned int)t[3]);
    unsigned int d = need > valid ? need - valid : valid - need;
    unsigned int hold, edo, x;

    if (d < (unsigned int)t[5] + 4)
        d = t[5] + 4;
    hold = clocks_minus1(d, ns);
    x = valid < (unsigned int)t[6] + 6 ? t[6] + 6 - valid : valid - t[6] - 6;
    edo = (x + ns - 1) / ns;
    if (edo > hold + 1 || edo * ns >= (unsigned int)t[7] + 6
        || setup > 7 || hold > 7 || edo > 7)
        return false;
    *timing = (setup << 16) | (hold << 12) | (edo << 28);
    return true;
}

/* Write setup/hold use the first three population timing bytes. */
static _Bool nand_calc_program_timing(const uint8_t *t, uint32_t *timing)
{
    const unsigned int ns = 1000 / NAND_BUS_MHZ;
    unsigned int setup = clocks_minus1(t[1] + 4, ns);
    unsigned int edge = ns * setup;
    unsigned int gap = edge < t[0] ? t[0] - edge : edge - t[0];
    unsigned int hold = clocks_minus1(MAX(gap, (unsigned int)t[2] + 4), ns);
    if (setup > 7 || hold > 7)
        return false;
    *timing = (setup << 16) | (hold << 12);
    return true;
}

/* Page 0 of a block: the dies of a chip enable follow each other, each
 * starting at a power-of-two multiple of its block count. */
uint32_t nand_block_row(unsigned int block)
{
    unsigned int per_die = nand_geom.blocks_per_die;
    unsigned int stride = nand_geom.die_stride;

    return nand_geom.page_stride
           * (stride * (block / per_die) + block % per_die);
}

/* Clocks and pins on, sequencer reset, chip reset + READ ID, match what
 * answered against the population table, fill nand_geom, compute the timing
 * and select the ECC mode. Idempotent. Returns 0, NAND_ERR_TIMEOUT
 * (sequencer did not answer), NAND_ERR_NOCHIP (nothing answered, or a part
 * not in the table) or NAND_ERR_UNSUPPORTED (a known part in a population
 * that is not driven). nand_get_id() and nand_get_raw_ids() are valid after
 * any return but TIMEOUT, for monitor diagnostics. */
int nand_phys_init(void)
{
    const struct nand_population *pop = NULL;
    unsigned int i;
    int rc;

    if (nand_phys_ready)
        return 0;

    /* Controller and ECC clocks on */
    clockgate_enable(CLOCKGATE_FMC, true);
    clockgate_enable(CLOCKGATE_FMC_ECC, true);

    /* Write enable, as the iPod OS before any program or erase (fw_disk
     * nand_power_set 0x08052ed0 -> 0x080562bc(1)): pin 8.6 high, then
     * 0x41a us before the bus pins. Apple's bootloader GPIO table drives
     * the same pin high (PCON8 nibble 6 = f); a DFU boot never runs it.
     * Device, Oct 25: without it every program and erase is ignored
     * (status e0, ready at once). */
    GPIOCMD = 0x8060f;
    udelay(0x41a);

    /* NAND bus pins: ports 6 and 7 pins to the controller function, port 8
     * pin 0 and pins 2..5 likewise, the other pins unchanged. */
    PCON(6) = (PCON(6) & 0xf) | 0x22222220;
    PCON(7) = 0x22222222;
    PCON(8) = (PCON(8) & 0xff0000f0) | 0x333302;

    /* _m2fmssInit (fw_disk 0x08054cc8) resets the sequencer before it
     * cleans the programs, and again inside the reset + READ ID step. */
    if (fmss_reset())
        return NAND_ERR_TIMEOUT;

    /* The sequencer fetches its programs from memory */
    clean_dcache_range((uint32_t)fmss_prog_readid, sizeof(fmss_prog_readid));
    clean_dcache_range((uint32_t)fmss_prog_read, sizeof(fmss_prog_read));
    clean_dcache_range((uint32_t)fmss_prog_program, sizeof(fmss_prog_program));
    clean_dcache_range((uint32_t)fmss_prog_bootread, sizeof(fmss_prog_bootread));
    clean_dcache_range((uint32_t)fmss_prog_erase, sizeof(fmss_prog_erase));

    if (nand_reset_and_read_ids(0xff))
        return NAND_ERR_TIMEOUT;

    /* bank i drives CE ce[i]; decode the first populated chip for display. */
    nand_ids_valid = true;
    nand_id_valid = false;
    for (i = 0; i < NAND_MAX_BANKS; i++)
    {
        if (nand_ce_empty(nand_idbuf[i]))
            continue;
        if (!nand_id_valid)
        {
            nand_decode_id(nand_idbuf[i], &nand_id_info);
            nand_id_valid = true;
        }
    }

    rc = nand_identify(nand_idbuf, &nand_geom, &pop);
    if (rc)
        return rc;
    if (!nand_calc_timing(pop->timing, &nand_timing))
        return NAND_ERR_UNSUPPORTED;

    if (!nand_calc_program_timing(pop->timing, &nand_program_timing))
        return NAND_ERR_UNSUPPORTED;

    /* ECC strength from the parity bytes the spare area has per sector (10
     * bytes of the spare area hold the metadata): 26 or more = BCH16
     * (26 parity bytes), 13 or more = BCH8 (13 parity bytes). ECC_CTRL bit 2
     * selects BCH16; 0xa00 includes the metadata in sector 0's codeword.
     * Length fields store byte count minus one. */
    if (nand_geom.ecc_bits == 16)
    {
        nand_ecc_mode = 0x5;  /* BCH16 decode */
        nand_ecc_cfg = 0xa05;  /* BCH16 decode + meta */
        nand_ecc_hdrlen = 26 + 9;  /* = 10 meta + 26 parity - 1 */
        nand_ecc_parlen = 26 - 1;
    }
    else
    {
        nand_ecc_mode = 0x1;  /* BCH8 decode */
        nand_ecc_cfg = 0xa01;  /* BCH8 decode + meta */
        nand_ecc_hdrlen = 13 + 9;  /* = 10 meta + 13 parity - 1 */
        nand_ecc_parlen = 13 - 1;
    }

    nand_phys_ready = true;
    return 0;
}

/* The eight READ ID words of the last detection (CE0..CE7, 0 or ~0 for an
 * empty chip enable), NULL before one ran */
const uint32_t *nand_get_raw_ids(void)
{
    return nand_ids_valid ? nand_idbuf : NULL;
}

/* Geometry of the detected chips, NULL until nand_phys_init() succeeded */
const struct nand_geometry *nand_get_geometry(void)
{
    return nand_phys_ready ? &nand_geom : NULL;
}

uint32_t nand_get_read_count(void)
{
    return nand_pages_read;
}

#include "nand-read.h"

#include "nand-write.h"
