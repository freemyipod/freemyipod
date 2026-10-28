; SPDX-License-Identifier: GPL-2.0-or-later
; Copyright (C) 2026
;
; Shared S5L87xx NAND mutation engine. Each descriptor is {command,row},
; terminated by command=0. command bytes: prior-status, setup, confirm, CE.
; P_ROWS points to descriptors; P_DMALIST to 2-KiB bus addresses; P_META to
; 12-byte metadata records (0 for boot pages). d7c is sectors per page.
; d48/d4c are write/read timings; d78 is the enhanced-status second-die row.
; ECC parameters describe the first and subsequent codewords independently.
; Setup 60h is erase (three address cycles, no data). Other setups program.
; Result r6: bit28 NAND failure, bit27 controller fault. d04: failing CE mask.
; Only d00..d7c is backed by the observed parameter RAM.
; Keep this layout in sync with nand-write.h (CPU completion wait).
; Scratch d50: used CEs, d54: command, d58: row, d5c: final CE, d60: probe,
; d64: status continuation (0=before a page, 1=final sweep).
;
; DMA, ECC and NAND transfers complete one sector at a time. Cache and
; multi-plane commands still overlap NAND array operations across pages.
    wr   [CS_IRQ], #0xff
    wr   [FMSS_C10], #1
    wr   [FMSS_C58], #4
    wr   [FMSS_C4C], #fault
    wr   [FMCTRL1], #0x33338e0
    wr   [FMC_STATUS], #0xff00ffe
    wr   [0xd04], #0
    wr   [0xd50], #0
    movi r6, #0
next_request:
    ld   r0, [P_ROWS]
    ldi  r5, [r0]
    bz   r5, drain
    st   [0xd54], r5
    add  r0, r0, #4
    ldi  r1, [r0]
    st   [0xd58], r1
    add  r0, r0, #4
    st   [P_ROWS], r0
    and  r0, r5, #0xff
    shl  r0, r0, #1
    ld   r1, [P_TIMING]
    or   r0, r1
    or   r0, r0, #0x801
    st   [FMC_CTRL0], r0
    wr   [0xd64], #0
    shr  r0, r5, #24
    bz   r0, begin_request
    st   [FMC_CMD], r0
    wait 0
    wr   [FMC_STATUS], #2
    sub  r0, r0, #0x78
    bnz  r0, poll_buffer
    ld   r0, [0xd58]
    st   [FMC_ADDR0], r0
    wr   [FMC_ANUM], #2
    wr   [FMCTRL1], #1
    wait 1
    wr   [FMC_STATUS], #4
poll_buffer:
    wr   [FMCTRL1], #0xca
    wait 12, arg=0x40, mask=0x40
status_result:
    ld   r0, [0x04c], 1
    wr   [FMCTRL1], #0x30020
    wr   [FMC_STATUS], #0x1800000
    bz   r0, status_ok
    and  r0, r5, #0xff
    ld   r1, [0xd04]
    or   r1, r0
    st   [0xd04], r1
    or   r6, r6, #0x10000000
status_ok:
    ld   r0, [0xd64]
    bnz  r0, next_probe
    bnz  r6, drain
begin_request:
    and  r0, r5, #0xff
    ld   r1, [0xd50]
    or   r1, r0
    st   [0xd50], r1
    shl  r0, r0, #1
    ld   r1, [0xd48]
    or   r0, r1
    or   r0, r0, #0x801
    st   [FMC_CTRL0], r0
    shr  r0, r5, #16
    and  r0, r0, #0xff
    st   [FMC_CMD], r0
    wait 0
    wr   [FMC_STATUS], #2
    sub  r0, r0, #0x60
    bz   r0, erase_address
    ld   r0, [0xd58]
    shl  r1, r0, #16
    st   [FMC_ADDR0], r1
    shr  r1, r0, #16
    st   [FMC_ADDR1], r1
    wr   [FMC_ANUM], #4
    wr   [FMCTRL1], #1
    wait 1
    wr   [FMC_STATUS], #4
    ld   r0, [P_META]
    bz   r0, data_setup
    ldi  r1, [r0]
    st   [FMC_DATA0?], r1
    add  r0, r0, #4
    ldi  r1, [r0]
    st   [FMC_64], r1
    add  r0, r0, #4
    ldi  r1, [r0]
    st   [FMC_68], r1
    add  r0, r0, #4
    st   [P_META], r0
    wr   [FMC_40], #0xc1
metadata_latch:
    ld   r0, [FMC_40], 1
    bnz  r0, metadata_latch
data_setup:
    ld   r0, [FMC_CTRL0]
    or   r0, r0, #0x1000000
    st   [FMC_CTRL0], r0
    wr   [0x038], #7
    wr   [0x028], #0
    movi r7, #0
sector:
    and  r0, r7, #3
    bnz  r0, dma_sector
    ld   r0, [P_DMALIST]
    ldi  r1, [r0]
    st   [FMC_DMA_ADDR], r1
    add  r0, r0, #4
    st   [P_DMALIST], r0
dma_sector:
    and  r4, r7, #1
    movi r0, #1
    shl  r0, r4
    st   [FMC_DATA_SEL], r0
    wr   [FMC_18], #1
    wr   [FMC_DNUM], #511
    wr   [FMCTRL1], #0x2e0
    wait 2
    wr   [FMC_STATUS], #0x100000
    ld   r2, [P_PARLEN]
    ld   r3, [P_ECCMODE]
    bnz  r7, encode_sector
    ld   r2, [P_HDRLEN]
    ld   r3, [P_ECCCFG]
encode_sector:
    wr   [ECC_STATUS], #0x1ff
    st   [FMC_1C], r4
    st   [ECC_CTRL], r3
    wait 4
    wr   [ECC_STATUS], #1
    movi r0, #0x1000
    shl  r0, r4
    st   [FMC_DATA_SEL], r0
    st   [FMC_DNUM], r2
    wr   [FMCTRL1], #0x34
    wait 3
    wr   [FMC_STATUS], #8
    movi r0, #0x100
    shl  r0, r4
    st   [FMC_DATA_SEL], r0
    wr   [FMC_DNUM], #511
    wr   [FMCTRL1], #0xe4
    wait 3
    wr   [FMC_STATUS], #8
    add  r7, r7, #1
    ld   r0, [0xd7c]
    sub  r0, r7
    bnz  r0, sector
confirm:
    shr  r0, r5, #8
    and  r0, r0, #0xff
    st   [FMC_CMD], r0
    wait 0
    wr   [FMC_STATUS], #2
    bnz  r5, next_request

erase_address:
    ld   r0, [0xd58]
    st   [FMC_ADDR0], r0
    wr   [FMC_ANUM], #2
    wr   [FMCTRL1], #1
    wait 1
    wr   [FMC_STATUS], #4
    bnz  r5, confirm

; Drain every selected CE even after a failure. Enhanced status checks both
; plane rows in both die address ranges, waiting for array-ready (bit5).
drain:
    wr   [0xd64], #1
    wr   [0xd5c], #1
select_final:
    ld   r5, [0xd5c]
    ld   r0, [0xd50]
    and  r0, r5
    bz   r0, next_ce
    shl  r0, r5, #1
    ld   r1, [P_TIMING]
    or   r0, r1
    or   r0, r0, #0x801
    st   [FMC_CTRL0], r0
    wr   [0xd60], #0
final_status:
    ld   r0, [0xd78]
    bnz  r0, enhanced
    wr   [FMC_CMD], #0x70
    wait 0
    wr   [FMC_STATUS], #2
    bnz  r5, poll_buffer
enhanced:
    wr   [FMC_CMD], #0x78
    wait 0
    wr   [FMC_STATUS], #2
    ld   r1, [0xd60]
    and  r2, r1, #2
    bz   r2, first_die
    movi r0, #0
first_die:
    and  r1, r1, #1
    shl  r1, r1, #7
    add  r0, r1
    st   [FMC_ADDR0], r0
    wr   [FMC_ANUM], #2
    wr   [FMCTRL1], #1
    wait 1
    wr   [FMC_STATUS], #4
    wr   [FMCTRL1], #0xca
    wait 12, arg=0x20, mask=0x20
    bnz  r5, status_result
next_probe:
    ld   r0, [0xd78]
    bz   r0, next_ce
    ld   r0, [0xd60]
    add  r0, r0, #1
    st   [0xd60], r0
    sub  r0, r0, #4
    bnz  r0, final_status
next_ce:
    shl  r5, r5, #1
    st   [0xd5c], r5
    and  r0, r5, #0xff
    bnz  r0, select_final
finish:
    ld   r0, [FMC_CTRL0], 0xfefffe01
    st   [FMC_CTRL0], r0
    end
fault:
    ld   r0, [FMC_CTRL0], 0x1fe
    shr  r0, r0, #1
    ld   r1, [0xd04]
    or   r1, r0
    st   [0xd04], r1
    wr   [FMSS_C50], #1
    or   r6, r6, #0x8000000
    bnz  r6, finish
