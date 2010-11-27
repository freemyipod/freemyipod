/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Free Software Foundation, Inc.
 * This file was originally part of the GNU C Library
 * Contributed to glibc by MontaVista Software, Inc. (written by Nicolas Pitre)
 * Adapted for Rockbox by Daniel Ankers
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

#define ASM_FILE
#include "global.h"

/* ARMv4T doesn't switch the T bit when popping pc directly, we must use BX */
.macro ldmpc cond="", order="ia", regs
#if ARM_ARCH == 4 && defined(USE_THUMB)
    ldm\cond\order sp!, { \regs, lr }
    bx\cond lr
#else
    ldm\cond\order sp!, { \regs, pc }
#endif
.endm
.macro ldrpc cond=""
#if ARM_ARCH == 4 && defined(USE_THUMB)
    ldr\cond lr, [sp], #4
    bx\cond  lr
#else
    ldr\cond pc, [sp], #4
#endif
.endm

/*
 * Endian independent macros for shifting bytes within registers.
 */
#ifndef __ARMEB__
#define pull            lsr
#define push            lsl
#else
#define pull            lsl
#define push            lsr
#endif

/* Prototype: void *memcpy(void *dest, const void *src, size_t n); */

    .section    .icode,"ax",%progbits

    .align      2
    .global     memcpy
    .type       memcpy,%function

memcpy:
        tst    r0, #31
        tsteq  r1, #31
        tsteq  r2, #31
        bne 2f

        stmfd   sp!, {r4-r8,lr}

1:      ldmia   r1!, {r3-r8,ip,lr}
        subs    r2, r2, #32
        stmia   r0!, {r3-r8,ip,lr}
        bhi 1b

	ldmpc   regs="r4-r8"

2:      tst    r0, #3
        tsteq  r1, #3
        tsteq  r2, #3
        bne 4f

3:      ldr     r3, [r1], #4
        subs    r2, r2, #4
        str     r3, [r0], #4
        bhi 3b

        bx lr

4:      ldrb    r3, [r1], #1
        subs    r2, r2, #1
        strb    r3, [r0], #1
        bhi 4b

        bx lr
