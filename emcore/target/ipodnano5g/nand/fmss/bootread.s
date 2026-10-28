; SPDX-License-Identifier: GPL-2.0-or-later
; Copyright (C) 2026
; Read the three BCH16 codewords of a 1536-byte boot page, without metadata.
; P_ROWS=row, P_CEMASKS=physical CE bit, P_DMALIST=destination bus address.
; Slow boot timing is independent of the detected population. r6 uses the
; normal read summary bits (30=UECC,29=erased,27=fault, low17=correction counts).
    wr   [CS_IRQ], #0xff
    wr   [FMSS_C10], #1
    wr   [FMSS_C58], #4
    wr   [FMSS_C4C], #fault
    wr   [FMCTRL1], #0x33338e0
    wr   [FMC_STATUS], #0xff00ffe
    wr   [0x808], #0x80
    movi r6, #0
    ld   r0, [P_CEMASKS]
    shl  r0, r0, #1
    or   r0, r0, #0x1077801
    st   [FMC_CTRL0], r0
    wr   [FMC_CMD], #0
    wait 0
    wr   [FMC_STATUS], #2
    ld   r0, [P_ROWS]
    shl  r1, r0, #16
    st   [FMC_ADDR0], r1
    shr  r1, r0, #16
    st   [FMC_ADDR1], r1
    wr   [FMC_ANUM], #4
    wr   [FMCTRL1], #1
    wait 1
    wr   [FMC_STATUS], #4
    wr   [FMC_CMD], #0x30
    wait 0
    wr   [FMC_STATUS], #2
    wr   [FMC_CMD], #0x70
    wait 0
    wr   [FMC_STATUS], #2
    wr   [FMCTRL1], #0xca
    wait 12, arg=0x40, mask=0x40
    wr   [FMCTRL1], #0x30020
    wr   [FMC_STATUS], #0x1800000
    wr   [FMC_CMD], #0
    wait 0
    wr   [FMC_STATUS], #2
    ld   r0, [P_DMALIST]
    st   [FMC_DMA_ADDR], r0
    wr   [0x038], #7
    wr   [0x028], #0
    movi r7, #3
sector:
    wr   [FMC_DATA_SEL], #0x10
    wr   [FMC_DNUM], #25
    wr   [FMCTRL1], #0x32
    wait 3
    wr   [FMC_STATUS], #8
    wr   [FMC_DATA_SEL], #1
    wr   [FMC_18], #1
    wr   [FMC_DNUM], #511
    wr   [FMCTRL1], #0xe2
    wait 3
    wr   [FMC_STATUS], #8
    wr   [FMC_1C], #0
    wr   [ECC_STATUS], #0x1ff
    wr   [ECC_CTRL], #5
    wait 4
    wr   [ECC_STATUS], #1
    ld   r0, [ECC_STATUS], 0x40
    bnz  r0, erased
    ld   r0, [ECC_RESULT], 1
    bnz  r0, uncorrectable
    ld   r0, [ECC_RESULT], 0x1f0000
    shr  r0, r0, #16
    movi r1, #1
    shl  r1, r0
    or   r6, r1
    bnz  r1, transfer
erased:
    or   r6, r6, #0x20000000
    bnz  r6, transfer
uncorrectable:
    or   r6, r6, #0x40000000
transfer:
    wr   [FMC_DATA_SEL], #0x100
    wr   [FMC_DNUM], #511
    wr   [FMCTRL1], #0x1e0
    wait 2
    wr   [FMC_STATUS], #0x100000
    sub  r7, r7, #1
    bnz  r7, sector
finish:
    ld   r0, [FMC_CTRL0], 0xfefffe01
    st   [FMC_CTRL0], r0
    end
fault:
    wr   [FMSS_C50], #1
    or   r6, r6, #0x8000000
    bnz  r6, finish
