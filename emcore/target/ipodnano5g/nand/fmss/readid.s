;             __________               __   ___.
;   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
;   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
;   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
;   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
;                     \/            \/     \/    \/            \/
;
; Copyright (C) 2026 Viktor Krapivin
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
; KIND, either express or implied.
;
;
; Read ID bytes 0..3 from P_ROWS chip enables into P_IDBUF (u32 per CE).
; An absent CE returns 0 or 0xffffffff.
; r6 returns 0, or 0x08000000 | selected CE mask on fault.
;
    wr   [CS_IRQ], #0xff            ; clear sequencer interrupts
    wr   [FMSS_C10], #0x1           ; sequencer enable (inf)
    wr   [FMSS_C58], #0x4           ; event timeout scale (inf)
    wr   [FMSS_C4C], #fault         ; fault vector
    wr   [FMCTRL1], #0x33338e0      ; transfer engine idle setup
    wr   [FMC_STATUS], #0xff00ffe   ; clear FMC status
    ld   r7, [P_IDBUF], 0xffffffff  ; r7 = output pointer
    ld   r6, [P_ROWS], 0xf          ; r6 = CEs left
    movi r5, #0x2                   ; r5 = chip enable bit of CE0
next_ce:
; ---- select one CE: FMC_CTRL0 bit (ce + 1), slow timing 0x77000 (pulse / hold
;      fields = 7), bit 0 set
    or   r0, r5, #0x77001
    st   [FMC_CTRL0], r0            ; select this CE, slow timing
; ---- command cycle 0x90 (READ ID)
    wr   [FMC_CMD], #0x90           ; READ ID
    wait 0                          ; command sent (FMC_STATUS bit 1)
    wr   [FMC_STATUS], #0x2         ; acknowledge (write 1 to clear)
; ---- address cycle: FMC_ANUM = number of cycles - 1
    wr   [FMC_ANUM], #0x0           ; one address cycle
    wr   [FMC_ADDR0], #0x0          ; address 0x00
    wr   [FMCTRL1], #0x1            ; send address
    wait 1                          ; address sent (FMC_STATUS bit 2)
    wr   [FMC_STATUS], #0x4
; ---- data cycles: FMC_DNUM = byte count - 1, routed into the FIFO (DATA_SEL 0x10)
    wr   [FMC_DNUM], #0x4           ; 5 bytes
    wr   [FMC_DATA_SEL], #0x10      ; into the FIFO
    wr   [FMCTRL1], #0xe2           ; read data
    wait 3                          ; data transferred (FMC_STATUS bit 3)
    wr   [FMC_STATUS], #0x8
; ---- FMC_40 = 0x52 latches the FIFO into FMC_60.. ; bit 1 reads 1 while busy
    wr   [FMC_40], #0x52            ; latch the FIFO
fifo_busy:
    ld   r4, [FMC_40], 0x2
    bnz  r4, fifo_busy
    ld   r0, [FMC_DATA0?], 0xffffffff ; FMC_60: ID bytes 0..3 (byte 0 = maker)
    sti  [r7], r0                   ; store ID bytes 0..3
    add  r7, r7, #0x4
    shl  r5, r5, #0x1               ; next CE select bit
    sub  r6, r6, #0x1
    bnz  r6, next_ce
    ld   r0, [FMC_CTRL0], 0xfffffe01 ; CE bits 8..1 cleared
    st   [FMC_CTRL0], r0            ; deselect
    movi r6, #0x0                   ; result: 0 = ok
    end
; ---- fault handler (same as reset.s): r6 = 0x08000000 | selected CE mask
fault:
    ld   r0, [FMC_CTRL0], 0x1fe
    shr  r0, r0, #0x1
    or   r6, r0, #0x8000000
    ld   r1, [FMC_CTRL0], 0xfffffe01
    st   [FMC_CTRL0], r1
    ld   r2, [FMSS_C3C], 0xffffffff ; fault info
    ld   r0, [FMSS_C50], 0x1
    wr   [FMSS_C50], #0x1           ; acknowledge the fault
    end
