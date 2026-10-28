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

/* NAND harness inside emCORE for the iPod nano 5G (S5L8730), on top of the Rockbox nano 5G NAND driver
 * (target/ipodnano5g/nand/, see nand5g.c for the job semantics). ONE worker thread executes one job at a time; the
 * USB sync commands 120..131 (usb/usbdebug.c, IRQ context) only queue a job and return. The host polls the status
 * record (struct n5g_nand_status, symbol `n5g_nand_status`; address/size come back from N5G_NAND_STATUS) with
 * READ_MEM and reads job output from the result buffer (status.result_addr, 64-byte aligned, status.result_cap
 * bytes). NOTHING touches the NAND at boot. Writes (commands 129..131) are built in unless N5G_NAND_WRITE=0.
 * tools/nano5g/nand.py mirrors the record layout (version N5G_NAND_VERSION).
 */
#ifndef N5G_NAND_H
#define N5G_NAND_H
#include "libc/include/stdint.h"

#define N5G_NAND_MAGIC    0x4e47354eu   /* "N5GN" little endian */
#define N5G_NAND_VERSION  1u

/* steps (N5G_NAND_STEP n, arg) */
#define N5G_STEP_GATES    1u   /* FMC + ECC clock gates open: PWRCON0 0x3c500048 bits 5 and 9 (clockgate_enable) */
#define N5G_STEP_PINS     2u   /* snapshot of the NAND pad words (the driver sets the pins itself in step 3) */
#define N5G_STEP_FIL      3u   /* nand_phys_init(): pins, chip reset, READ ID per CE, chip table match, timing -> IDs/geometry */
#define N5G_STEP_FTL      4u   /* ftl_init(): VFL + FTL mount -> ftl_pages (4 KiB sectors) */
#define N5G_NSTEPS        8u

/* Job IDs are stable; retired ID 5 is reserved. */
#define N5G_JOB_NONE      0u
#define N5G_JOB_STEP      1u   /* arg0 = step, arg1 = step argument */
#define N5G_JOB_READ_ROM  2u   /* arg0 = page (bank 0), arg1 = 1: fmc_reset_bank(0) first: ROM-format page, 0x600 bytes */
#define N5G_JOB_READ_PAGE 3u   /* arg0 = bank (CE index), arg1 = page (row) */
#define N5G_JOB_FTL_READ  4u   /* arg0 = lpn, arg1 = count: FTL pages of ftl_bpp bytes (WMR 0x65) */
#define N5G_JOB_BLOCK_READ 6u  /* arg0 = lba, arg1 = count: 4 KiB blocks of the BlockIo view (what the bootloader sees) */

/* USB monitor commands (usbdebug.c). Retired command 126 is reserved. */
#define N5G_NAND_STATUS     120   /* -> {1, &n5g_nand_status, size, result buffer} */
#define N5G_NAND_STEP       121   /* (step, arg) */
#define N5G_NAND_READ_ROM   122   /* (page, reset_first) */
#define N5G_NAND_READ_PAGE  123   /* (bank, page) */
#define N5G_NAND_FTL_OPEN   124
#define N5G_NAND_FTL_READ   125   /* (lpn, count) */
#define N5G_NAND_BLOCK_READ 127   /* (lba, count) */

/* Optional writer protocol v1. Capabilities are available even in read-only builds.
 * Write staging uses the result buffer; checksum is standard CRC-32.
 * A failed mutating job poisons the writer until reboot. Never replay a command
 * after a transport timeout: its outcome is unknown. */
#define N5G_NAND_WRITE_CAPS 128  /* -> {protocol=1, flags(bit0=writer), max blocks} */
#define N5G_NAND_WRITE_OPEN 129 /* requires read-only FTL open; may recover/store metadata */
#define N5G_NAND_BLOCK_WRITE 130 /* {lba4k, count, CRC32(staged bytes)} */
#define N5G_NAND_FLUSH 131
#define N5G_JOB_WRITE_OPEN 7u
#define N5G_JOB_BLOCK_WRITE 8u
#define N5G_JOB_FLUSH 9u

/* job states */
#define N5G_STATE_IDLE     0u
#define N5G_STATE_QUEUED   1u
#define N5G_STATE_RUNNING  2u   /* record written before the first register access */
#define N5G_STATE_DONE     3u

/* job_result (harness outcome; job_lib_rc carries the library's own return code) */
#define N5G_R_OK        0u
#define N5G_R_LIB_ERROR 1u
#define N5G_R_PRECOND   2u      /* prerequisite step missing: nothing touched */
#define N5G_R_BAD_ARG   3u
#define N5G_R_NO_MEMORY 4u
#define N5G_R_PANIC     5u      /* the library reported a panic / assert during the job (hal->panic) */

/* accept codes (reply word 1 of the queuing commands) */
#define N5G_ACCEPT_OK       0u
#define N5G_ACCEPT_BUSY     1u
#define N5G_ACCEPT_BAD_ARG  2u
#define N5G_ACCEPT_NOT_INIT 3u

#define N5G_STEP_NOT_RUN  0xffffffffu
#define N5G_STEP_RUNNING  0xfffffffeu

#define N5G_RESULT_BYTES  (256u * 1024u)
#define N5G_FTL_CHUNK     8u    /* FTL pages per WMR_Dispatch(0x65) call */

/* READ_PAGE result layout inside the result buffer (spare room for 8 KiB pages) */
#define N5G_RP_DATA     0x0000u   /* page data: bytes_per_page (4096 / 8192) */
#define N5G_RP_META     0x2000u   /* 12 meta bytes (10 ECC-protected + FF FF), then 4 zero pad bytes */
#define N5G_RP_MAXCORR  0x2010u   /* 1 byte: max corrected bits + 1, 15 zero bytes */
#define N5G_RP_SECSTAT  0x2020u   /* 16 bytes: per-512-B-sector verdict (0..16 corrected bits, 0xfe erased, 0xff uncorrectable) */
#define N5G_RP_BYTES    0x2040u

struct n5g_nand_status {
    uint32_t magic, version, size;                 /* 0x000 */
    uint32_t busy;                                 /* 0x00c 1 = a job is queued or running */
    uint32_t state;                                /* 0x010 N5G_STATE_* of the current/last job */
    uint32_t step_reached;                         /* 0x014 highest step whose last run succeeded */
    uint32_t step_ok_mask;                         /* 0x018 bit n = step n succeeded */
    uint32_t jobs_started, jobs_done;              /* 0x01c, 0x020 sequence numbers (job id = jobs_started) */
    uint32_t job_kind;                             /* 0x024 */
    uint32_t job_arg[3];                           /* 0x028 */
    uint32_t job_result;                           /* 0x034 N5G_R_* */
    uint32_t job_lib_rc;                           /* 0x038 raw library return value */
    uint32_t job_detail[4];                        /* 0x03c job-specific  */
    uint32_t job_t_start, job_t_end;               /* 0x04c us counter */
    uint32_t progress_done, progress_total;        /* 0x054 */
    uint32_t step_status[N5G_NSTEPS];              /* 0x05c per-step raw code */
    uint32_t chip_ids[8];                          /* 0x07c READ ID words per physical CE (m2 idbuf; 98 d7 94 32 -> 0x3294d798) */
    uint32_t geom[16];                             /* 0x09c FIL_GetDeviceInfo: [0] pages/block 0x64, [1] nCE 0x65, [2] vendor 0x66,
                                                      [3] blocks/CE 0x67, [4] bytes/page 0x68, [5] spare 0x69, [6] cfg word 0x6b,
                                                      [7] refresh thr 0x6c, [8] ECC bits 0x73, [9] sector 0x74, [10] boot page 0x72,
                                                      [11] di.chipid, [12] dies 0x76, [13] die stride 0x78, [14] total blocks 0x7a,
                                                      [15] FMC timing word (FIL context timing_ctrl0 = rSetup<<16 | rHold<<12 | rEdo<<28, written to FMSS param RAM 0xd4c) */
    uint32_t ftl_pages, ftl_bpp;                   /* 0x0dc dwUserPagesTotal / bytes per FTL page (WMR 0x70) */
    uint32_t blockio_blocks;                       /* 0x0e4 4 KiB blocks of the BlockIo view (0 = not open) */
    uint32_t result_addr, result_cap, result_len;  /* 0x0e8 */
    uint32_t clk_mhz;                              /* 0x0f4 FMC clock handed to FIL_CalcTiming (137 = HCLK, inferred) */
    uint32_t pwrcon0_before, pwrcon0_after;        /* 0x0f8 step 1 */
    uint32_t gpio_after[3];                        /* 0x100 0x3cf000c0/e0/100 after step 2 */
    uint32_t fil_pages, fil_uecc, fil_timeouts;    /* 0x10c live FIL statistics (low 32 bits) */
    uint32_t alloc_bytes, alloc_count, alloc_fail; /* 0x118 hal->alloc */
    uint32_t panic_count, panic_line, panic_t_us;  /* 0x124 last hal->panic */
    char     panic_file[32];                       /* 0x130 */
    uint32_t wmr_asserts;                          /* 0x150 wmr_assert_failures() */
    uint32_t err_job_seq;                          /* 0x154 job id of the last failed job */
    uint32_t snap_valid;                           /* 0x158 1 = the register snapshot below belongs to err_job_seq */
    uint32_t fmc_snap[24];                         /* 0x15c FMC 0x38a00000..0x38a0005c */
    uint32_t ecc_snap[6];                          /* 0x1bc FMC 0x38a00800..0x38a00814 */
    uint32_t fmss_snap[28];                        /* 0x1d4 FMSS 0x38a00c00..0x38a00c6c */
    uint32_t now_us;                               /* 0x244 refreshed by N5G_NAND_STATUS */
    uint32_t hb_word;                              /* 0x248 heartbeat word 0 */
    uint32_t reserved;                             /* 0x24c */
};
#define N5G_STATUS_BYTES 0x250u

extern volatile struct n5g_nand_status n5g_nand_status;

/* boot (init.c, after USB): create the worker thread and the result buffer. No hardware access. */
int  n5g_nand_init(void);
/* usbdebug.c sync handler (IRQ context): queue a job / report the status record. buf = request words,
 * reply = dbgbuf (reply[0] = 1, reply[1..3] command-specific). */
void n5g_nand_usb_command(const uint32_t *buf, uint32_t *reply);
/* heartbeat (usbdebug cmd 200): w0 = bit31 busy | state<<24 | job_kind<<16 | step_reached<<8 | jobs_done & 0xff,
 * w1 = FIL pages read since boot (live progress). */
void n5g_nand_heartbeat(uint32_t *w0, uint32_t *w1);

#endif
