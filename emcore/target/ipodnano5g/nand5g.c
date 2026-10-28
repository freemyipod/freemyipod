/*
 * Copyright (C) 2026 Vikotr Krapivin
 *
 * This file is part of emCORE.
 *
 * emCORE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * emCORE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with emCORE. If not, see <http://www.gnu.org/licenses/>.
 */

//
/* Nano5g NAND worker. USB commands queue jobs and return immediately;
 * hosts poll the status record and retrieve data from the result buffer.
 * Boot only creates the worker. Physical initialization and FTL mounting
 * occur on explicit commands. Writes use staged data with a CRC-32. */
#include "global.h"
#include "thread.h"
#include "timer.h"
#include "malloc.h"
#include "console.h"
#include "interrupt.h"
#include "clockgates.h"
#include "crc32.h"
#include "nand5g.h"
#include "module5g.h"
/* the driver's public headers are shaped by the Rockbox model define */
#include "nand/nand-target.h"
#include "nand/ftl-target.h"

volatile struct n5g_nand_status n5g_nand_status;
#define S n5g_nand_status

_Static_assert(sizeof(struct n5g_nand_status) == N5G_STATUS_BYTES, "status record layout (nano5g/nand.py mirrors it)");

#define REG32(a) (*(volatile uint32_t *)(a))
#define N5G_FMC_BASE   0x38a00000u
#define N5G_GATE0      0x3c500048u
#define N5G_GATE_FMC   5
#define N5G_GATE_ECC   9
#define N5G_GPIO_BASE  0x3cf00000u
#define DLINE 32u
#define WHOLE_CACHE_BYTES 0x8000u

static struct scheduler_thread g_thread;
static struct wakeup g_wake;
static uint8_t *g_result;                 /* N5G_RESULT_BYTES, 64-byte aligned */
static uint8_t *g_meta;                   /* 64 B: 12 meta bytes of a page read (DMA target) */
static int g_ready, g_ftl_ready;
static uint32_t g_ftl_pages;
static struct { uint32_t kind, arg[3]; } g_job;

/* ------------------------------------------------------------------ cache helpers (ARM1176) */
static void cp15_dsb(void) { __asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0) : "memory"); }

static void cache_wb(void *addr, uint32_t len)
{
    if (len >= WHOLE_CACHE_BYTES)
        __asm__ volatile("mcr p15, 0, %0, c7, c10, 0" : : "r"(0) : "memory");
    else {
        uint32_t a = (uint32_t)addr & ~(DLINE - 1u), e = (uint32_t)addr + len;
        for (; a < e; a += DLINE) __asm__ volatile("mcr p15, 0, %0, c7, c10, 1" : : "r"(a) : "memory");
    }
    cp15_dsb();
}

/* ------------------------------------------------------------------ status record */
static void publish(void) { cache_wb((void *)&n5g_nand_status, sizeof(n5g_nand_status)); }

static void refresh_live(void)
{
    S.fil_pages = nand_get_read_count();
    S.now_us = read_usec_timer();
    S.hb_word = (S.busy ? 0x80000000u : 0u) | (S.state & 0x7fu) << 24 | (S.job_kind & 0xffu) << 16 |
                (S.step_reached & 0xffu) << 8 | (S.jobs_done & 0xffu);
}

static void snapshot_regs(void)
{
    uint32_t i;
    if (!(S.step_ok_mask & (1u << N5G_STEP_GATES))) { S.snap_valid = 0; return; }   /* a gated block may not answer */
    for (i = 0; i < 24; i++) S.fmc_snap[i] = REG32(N5G_FMC_BASE + 4u * i);
    for (i = 0; i < 6; i++) S.ecc_snap[i] = REG32(N5G_FMC_BASE + 0x800u + 4u * i);
    for (i = 0; i < 28; i++) S.fmss_snap[i] = REG32(N5G_FMC_BASE + 0xc00u + 4u * i);
    S.snap_valid = 1;
}

static void geom_from_driver(void)
{
    const struct nand_geometry *g = nand_get_geometry();
    const uint32_t *ids = nand_get_raw_ids();
    uint32_t i;
    for (i = 0; i < 8; i++) S.chip_ids[i] = ids && i < NAND_MAX_BANKS ? ids[i] : 0;
    if (!g) return;
    S.geom[0] = g->pages_per_block;
    S.geom[1] = g->banks;
    S.geom[2] = g->vendor;
    S.geom[3] = g->blocks;
    S.geom[4] = g->page_size;
    S.geom[5] = g->spare_size;
    S.geom[6] = g->layout;              /* NAND_LAYOUT_* of the population (was: Apple cfg word) */
    S.geom[7] = g->refresh_threshold;
    S.geom[8] = g->ecc_bits;
    S.geom[9] = 512;
    S.geom[10] = 1536;                  /* boot page: three BCH16 sectors */
    S.geom[11] = g->id;
    S.geom[12] = g->dies;
    S.geom[13] = g->die_stride;
    S.geom[14] = g->blocks * g->banks;
    S.geom[15] = 0;                     /* FMC timing word: inside the driver (nand_calc_timing) */
}

/* ------------------------------------------------------------------ steps */
static int need(uint32_t mask) { return (S.step_ok_mask & mask) == mask; }

static uint32_t step_gates(void)
{
    S.pwrcon0_before = REG32(N5G_GATE0);
    clockgate_enable(N5G_GATE_FMC, true);
    clockgate_enable(N5G_GATE_ECC, true);
    S.pwrcon0_after = REG32(N5G_GATE0);
    return (S.pwrcon0_after & ((1u << N5G_GATE_FMC) | (1u << N5G_GATE_ECC))) ? 1u : 0u;
}

static uint32_t step_pins(uint32_t arg)
{
    (void)arg;
    S.gpio_after[0] = REG32(N5G_GPIO_BASE + 0xc0);
    S.gpio_after[1] = REG32(N5G_GPIO_BASE + 0xe0);
    S.gpio_after[2] = REG32(N5G_GPIO_BASE + 0x100);
    return 0;
}

static uint32_t step_phys(void)
{
    int rc = nand_phys_init();
    S.clk_mhz = 137;                    /* NAND_BUS_MHZ of the driver */
    geom_from_driver();
    S.gpio_after[0] = REG32(N5G_GPIO_BASE + 0xc0);
    S.gpio_after[1] = REG32(N5G_GPIO_BASE + 0xe0);
    S.gpio_after[2] = REG32(N5G_GPIO_BASE + 0x100);
    S.job_detail[0] = S.geom[1];
    S.job_detail[1] = (uint32_t)rc;
    return (uint32_t)rc;
}

static uint32_t step_ftl(void)
{
    int rc;
    if (g_ftl_ready) return 0;
    rc = ftl_init();
    if (rc == 0) {
        g_ftl_pages = ftl_num_sectors();
        S.ftl_pages = g_ftl_pages;
        S.ftl_bpp = 4096;
        S.blockio_blocks = g_ftl_pages;
        S.job_detail[0] = g_ftl_pages;
        S.job_detail[1] = 4096;
        S.job_detail[2] = ftl_was_restored();
        g_ftl_ready = g_ftl_pages != 0;
        if (!g_ftl_ready) rc = NAND_ERR_FTL;
    }
    return (uint32_t)rc;
}

/* ------------------------------------------------------------------ jobs */
static uint32_t job_step(uint32_t step, uint32_t arg, uint32_t *result)
{
    uint32_t rc;
    if (step == 0 || step > N5G_STEP_FTL) { *result = N5G_R_BAD_ARG; return 0; }
    if ((step == N5G_STEP_FIL && !need((1u << N5G_STEP_GATES) | (1u << N5G_STEP_PINS))) ||
        (step == N5G_STEP_FTL && !need(1u << N5G_STEP_FIL)) ||
        (step == N5G_STEP_FIL && g_ftl_ready)) { *result = N5G_R_PRECOND; return 0; }
    S.step_status[step] = N5G_STEP_RUNNING;
    publish();
    switch (step) {
    case N5G_STEP_GATES: rc = step_gates(); break;
    case N5G_STEP_PINS:  rc = step_pins(arg); break;
    case N5G_STEP_FIL:   rc = step_phys(); break;
    default:             rc = step_ftl(); break;
    }
    S.step_status[step] = rc;
    if (rc == 0) {
        S.step_ok_mask |= 1u << step;
        if (step > S.step_reached) S.step_reached = step;
    } else {
        S.step_ok_mask &= ~(1u << step);
        *result = N5G_R_LIB_ERROR;
    }
    return rc;
}

/* Boot-area page of bank 0 (three BCH16 sectors, 1536 bytes). The host's ROM result codes: 0 blank, 1 ok,
 * 3 uncorrectable, 5 timeout. */
static uint32_t job_read_rom(uint32_t page, uint32_t reset_first, uint32_t *result)
{
    int rc;
    uint32_t code;
    (void)reset_first;                          /* the driver resets the chips in nand_phys_init */
    if (!need(1u << N5G_STEP_FIL)) { *result = N5G_R_PRECOND; return 0; }
    memset(g_result, 0xa5, 0x600);
    cache_wb(g_result, 0x600);
    rc = nand_read_boot_page(0, page, g_result);
    code = rc == NAND_OK ? 1u : rc == NAND_ERASED ? 0u : rc == NAND_ERR_ECC ? 3u : rc == NAND_ERR_TIMEOUT ? 5u : (uint32_t)rc;
    S.job_detail[0] = code;
    S.job_detail[1] = (uint32_t)rc;
    S.result_len = 0x600;
    if (code != 1u) *result = N5G_R_LIB_ERROR;
    return code;
}

static uint32_t job_read_page(uint32_t bank, uint32_t page, uint32_t *result)
{
    const struct nand_geometry *g = nand_get_geometry();
    uint8_t verdict;
    uint32_t i;
    int rc;
    if (!need(1u << N5G_STEP_FIL) || !g) { *result = N5G_R_PRECOND; return 0; }
    if (bank >= g->banks || page >= g->pages_per_block * g->block_span) { *result = N5G_R_BAD_ARG; return 0; }
    memset(g_meta, 0xa5, 64);
    cache_wb(g_meta, 64);
    rc = nand_read_page(bank, page, g_result + N5G_RP_DATA, g_meta);
    for (i = 0; i < 16; i++) g_result[N5G_RP_META + i] = i < NAND_META_SIZE ? g_meta[i] : 0;
    for (i = 0; i < 16; i++) g_result[N5G_RP_MAXCORR + i] = 0;
    /* the driver reports one verdict per page: ok, erased or uncorrectable */
    verdict = rc == NAND_OK ? 0 : rc == NAND_ERASED ? 0xfe : 0xff;
    for (i = 0; i < 16; i++) g_result[N5G_RP_SECSTAT + i] = i < g->page_size / 512 ? verdict : 0;
    S.job_detail[0] = (uint32_t)rc;
    S.job_detail[1] = 0;
    S.result_len = N5G_RP_BYTES;
    if (rc != NAND_OK && rc != NAND_ERASED) *result = N5G_R_LIB_ERROR;
    return (uint32_t)rc;
}

static uint32_t job_ftl_read(uint32_t lpn, uint32_t count, uint32_t *result)
{
    uint32_t done = 0, worst = 0;
    if (!g_ftl_ready) { *result = N5G_R_PRECOND; return 0; }
    if (count == 0 || count * 4096u > N5G_RESULT_BYTES || lpn >= g_ftl_pages || count > g_ftl_pages - lpn) {
        *result = N5G_R_BAD_ARG; return 0;
    }
    S.progress_total = count;
    S.job_detail[1] = 0xffffffffu;
    while (done < count) {
        uint32_t n = count - done > N5G_FTL_CHUNK ? N5G_FTL_CHUNK : count - done;
        int rc = ftl_read(lpn + done, n, g_result + done * 4096u);
        if (rc != 0) {
            worst = (uint32_t)rc;
            if (S.job_detail[1] == 0xffffffffu) S.job_detail[1] = lpn + done;
            break;
        }
        done += n;
        S.progress_done = done;
        publish();
    }
    S.job_detail[0] = worst;
    S.result_len = done * 4096u;
    if (worst != 0) *result = N5G_R_LIB_ERROR;
    return worst;
}

#if N5G_NAND_WRITE
static int g_write_ready, g_write_poisoned;
static uint32_t job_write(uint32_t kind, const uint32_t *arg, uint32_t *result)
{
    int rc = 0;
    if (!g_ftl_ready || g_write_poisoned || (kind != N5G_JOB_WRITE_OPEN && !g_write_ready)) {
        *result = N5G_R_PRECOND; return 0;
    }
    if (kind == N5G_JOB_BLOCK_WRITE) {
        uint32_t lba = arg[0], count = arg[1];
        if (!count || count > N5G_RESULT_BYTES / 4096u || lba >= g_ftl_pages ||
            count > g_ftl_pages - lba || crc32(g_result, count * 4096u) != arg[2]) {
            *result = N5G_R_BAD_ARG; return 0;
        }
        S.progress_total = count;
        rc = ftl_write(lba, count, g_result);
        if (!rc) S.progress_done = count;
    } else if (kind == N5G_JOB_WRITE_OPEN) {
        rc = nand_write_supported() ? 0 : NAND_ERR_UNSUPPORTED;
        if (!rc) g_write_ready = 1;
    } else rc = ftl_flush();
    if (rc) { g_write_poisoned = 1; *result = N5G_R_LIB_ERROR; }
    return (uint32_t)rc;
}
#endif

static void run_job(uint32_t kind, const uint32_t *arg)
{
    uint32_t result = N5G_R_OK, rc = 0;
    S.state = N5G_STATE_RUNNING;               /* record BEFORE the first register access */
    S.job_t_start = read_usec_timer();
    S.job_t_end = 0;
    S.progress_done = 0;
    S.progress_total = 1;
    S.result_len = 0;
    publish();

    switch (kind) {
    case N5G_JOB_STEP:       rc = job_step(arg[0], arg[1], &result); break;
    case N5G_JOB_READ_ROM:   rc = job_read_rom(arg[0], arg[1], &result); break;
    case N5G_JOB_READ_PAGE:  rc = job_read_page(arg[0], arg[1], &result); break;
    case N5G_JOB_FTL_READ:
    case N5G_JOB_BLOCK_READ: rc = job_ftl_read(arg[0], arg[1], &result); break;
#if N5G_NAND_WRITE
    case N5G_JOB_WRITE_OPEN:
    case N5G_JOB_BLOCK_WRITE:
    case N5G_JOB_FLUSH:      rc = job_write(kind, arg, &result); break;
#endif
    default:                 result = N5G_R_BAD_ARG; break;
    }
    if (result != N5G_R_OK && result != N5G_R_PRECOND && result != N5G_R_BAD_ARG) snapshot_regs();
    if (S.result_len) cache_wb(g_result, S.result_len);
    S.job_lib_rc = rc;
    S.job_result = result;
    if (result != N5G_R_OK) S.err_job_seq = S.jobs_started;
    if (S.progress_total == 1 && result == N5G_R_OK) S.progress_done = 1;
    S.job_t_end = read_usec_timer();
    refresh_live();
    S.state = N5G_STATE_DONE;
    S.jobs_done = S.jobs_started;
    S.busy = 0;
    publish();
}

static void worker(void *a0, void *a1, void *a2, void *a3)
{
    (void)a0; (void)a1; (void)a2; (void)a3;
    while (1) {
        uint32_t kind, arg[3], mode;
        wakeup_wait(&g_wake, TIMEOUT_BLOCK);
        mode = enter_critical_section();
        kind = g_job.kind; arg[0] = g_job.arg[0]; arg[1] = g_job.arg[1]; arg[2] = g_job.arg[2];
        g_job.kind = N5G_JOB_NONE;
        leave_critical_section(mode);
        if (kind != N5G_JOB_NONE) run_job(kind, arg);
    }
}

/* ------------------------------------------------------------------ public */
int n5g_nand_init(void)
{
    uint32_t i;
    void *stack;
    if (g_ready) return 0;
    for (i = 0; i < N5G_NSTEPS; i++) S.step_status[i] = N5G_STEP_NOT_RUN;
    S.magic = N5G_NAND_MAGIC;
    S.version = N5G_NAND_VERSION;
    S.size = N5G_STATUS_BYTES;
    g_result = memalign(64, N5G_RESULT_BYTES);
    g_meta = memalign(64, 64);
    stack = memalign(32, 0x8000);
    if (!g_result || !g_meta || !stack) {
        cprintf(CONSOLE_BOOT, "[nand] harness: out of memory\n");
        return -1;
    }
    reownalloc(g_result, KERNEL_OWNER(KERNEL_OWNER_UNKNOWN));
    reownalloc(g_meta, KERNEL_OWNER(KERNEL_OWNER_UNKNOWN));
    reownalloc(stack, KERNEL_OWNER(KERNEL_OWNER_UNKNOWN));
    memset(g_result, 0, N5G_RESULT_BYTES);
    cache_wb(g_result, N5G_RESULT_BYTES);
    S.result_addr = (uint32_t)g_result;
    S.result_cap = N5G_RESULT_BYTES;
    wakeup_init(&g_wake);
    /* below the USB monitor worker: a long FTL mount leaves the async monitor responsive */
    if (!thread_create(&g_thread, "nand worker", worker, stack, 0x8000, CORE_THREAD, 64, true, NULL, NULL, NULL, NULL)) {
        cprintf(CONSOLE_BOOT, "[nand] harness: thread_create failed\n");
        return -1;
    }
    g_ready = 1;
    publish();
    return 0;
}

void n5g_nand_heartbeat(uint32_t *w0, uint32_t *w1)
{
    if (n5g_module_heartbeat(w0, w1)) return;
    *w0 = (S.busy ? 0x80000000u : 0u) | (S.state & 0x7fu) << 24 | (S.job_kind & 0xffu) << 16 |
          (S.step_reached & 0xffu) << 8 | (S.jobs_done & 0xffu);
    *w1 = nand_get_read_count();
}

static uint32_t queue(uint32_t kind, uint32_t a0, uint32_t a1, uint32_t a2)
{
    uint32_t mode, i;
    if (!g_ready) return N5G_ACCEPT_NOT_INIT;
    mode = enter_critical_section();
    if (n5g_module_blocks_nand()) { leave_critical_section(mode); return N5G_ACCEPT_BUSY; }
    if (S.busy) { leave_critical_section(mode); return N5G_ACCEPT_BUSY; }
    S.busy = 1;
    S.state = N5G_STATE_QUEUED;
    S.jobs_started++;
    S.job_kind = kind;
    S.job_arg[0] = a0; S.job_arg[1] = a1; S.job_arg[2] = a2;
    S.job_result = 0; S.job_lib_rc = 0;
    for (i = 0; i < 4; i++) S.job_detail[i] = 0;
    g_job.kind = kind; g_job.arg[0] = a0; g_job.arg[1] = a1; g_job.arg[2] = a2;
    leave_critical_section(mode);
    wakeup_signal(&g_wake);
    return N5G_ACCEPT_OK;
}

void n5g_nand_usb_command(const uint32_t *buf, uint32_t *reply)
{
    uint32_t acc;
    if (n5g_module_dispatch(buf, reply)) return;
    reply[0] = 1;
    switch (buf[0]) {
    case N5G_NAND_WRITE_CAPS:
        reply[1] = 1; reply[2] = N5G_NAND_WRITE ? 1 : 0;
        reply[3] = N5G_RESULT_BYTES / 4096u; return;
#if N5G_NAND_WRITE
    case N5G_NAND_WRITE_OPEN: acc = queue(N5G_JOB_WRITE_OPEN, 0, 0, 0); break;
    case N5G_NAND_BLOCK_WRITE: acc = queue(N5G_JOB_BLOCK_WRITE, buf[1], buf[2], buf[3]); break;
    case N5G_NAND_FLUSH: acc = queue(N5G_JOB_FLUSH, 0, 0, 0); break;
#endif
    case N5G_NAND_STATUS:     /* -> {status record address, size, result buffer address} */
        if (g_ready) refresh_live();
        reply[1] = (uint32_t)&n5g_nand_status;
        reply[2] = N5G_STATUS_BYTES;
        reply[3] = S.result_addr;
        return;
    case N5G_NAND_STEP:       acc = queue(N5G_JOB_STEP, buf[1], buf[2], 0); break;
    case N5G_NAND_READ_ROM:   acc = queue(N5G_JOB_READ_ROM, buf[1], buf[2], 0); break;
    case N5G_NAND_READ_PAGE:  acc = queue(N5G_JOB_READ_PAGE, buf[1], buf[2], 0); break;
    case N5G_NAND_FTL_OPEN:   acc = queue(N5G_JOB_STEP, N5G_STEP_FTL, 0, 0); break;
    case N5G_NAND_FTL_READ:   acc = queue(N5G_JOB_FTL_READ, buf[1], buf[2], 0); break;
    case N5G_NAND_BLOCK_READ: acc = queue(N5G_JOB_BLOCK_READ, buf[1], buf[2], 0); break;
    default: reply[0] = 2; return;
    }
    reply[1] = acc;
    reply[2] = S.jobs_started;            /* job id when accepted */
    reply[3] = (uint32_t)&n5g_nand_status;
}


int n5g_nand_module_can_load(void)
{
    return g_ready && !S.busy && !S.jobs_started;
}
