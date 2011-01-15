@
@
@    emCORE Loader for iPod Classic
@
@    Copyright 2010 TheSeven
@
@
@    This file is part of emCORE.
@
@    emCORE is free software: you can redistribute it and/or
@    modify it under the terms of the GNU General Public License as
@    published by the Free Software Foundation, either version 2 of the
@    License, or (at your option) any later version.
@
@    emCORE is distributed in the hope that it will be useful,
@    but WITHOUT ANY WARRANTY; without even the implied warranty of
@    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
@    See the GNU General Public License for more details.
@
@    You should have received a copy of the GNU General Public License along
@    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
@
@


#include "build/version.h"


.global _start
_start:

	msr	cpsr_c, #0xd3
	mrc	p15, 0, r0,c1,c0
	bic	r0, r0, #1
	mcr	p15, 0, r0,c1,c0
	mov	r0, #0
	mcr	p15, 0, r0,c7,c5
	mcr	p15, 0, r0,c8,c7
	mcr	p15, 0, r0,c13,c0
	adr	lr, cacheflush_done

flushcache:
	mrc	p15, 0, r15,c7,c14,3
	bne	flushcache
	mov	r0, #0
	mcr	p15, 0, r0,c7,c10,4
	bx	lr

cacheflush_done:
	adr	sp, values1
	ldmia	sp!, {r4-r12}
	mcr	p15, 0, r11,c2,c0
	mov	r0, #-1
	mcr	p15, 0, r0,c3,c0
mmuloop:
	str	r12, [r11], #4
	add	r12, r12, #0x00100000
	cmp	r12, #0x38000000
	biccs	r12, r12, #0xc
	tst	r12, #0x40000000
	beq	mmuloop
	mcr	p15, 0, r10,c1,c0
dmaloop:
	add	r8, r8, #0x10
	bic	r9, r9, r4
	cmp	r4, r5
	movcs	r4, r5
	movcs	r8, #0
	orr	r9, r9, r4
	stmia	r11!, {r6-r9}
	add	r6, r6, r4,lsl#1
	subs	r5, r5, r4
	bne	dmaloop
	ldmia	sp!, {r0-r12}        @ R0 = SYSCONBASE (temporary)
	str	r1, [r0,#0x48]
	str	r2, [r0,#0x4c]
	mov	r2, #0               @ R2 = 0
	str	r2, [r0]
	str	r2, [r0,#0x4]
	mov	r1, #7
	str	r1, [r0,#0x44]
	mov	r1, #0x30
	str	r1, [r0,#0x44]
	str	r2, [r0,#0x60]
	str	r3, [r0,#0x28]
	str	r4, [r0,#0x38]
	mov	r1, #0x34
	str	r1, [r0,#0x44]
	orr	r1, r1, #0x40000
	str	r1, [r0,#0x44]
waitpll:
	ldr	r1, [r0,#0x40]
	tst	r1, #4
	beq	waitpll
	str	r5, [r0,#0x04]
	mov	r1, #0x3000
	str	r1, [r0]
	mov	r1, #0x8000
	orr	r3, r1, #0x80000000
	str	r3, [r0,#0x8]
	str	r3, [r0,#0xc]
	str	r1, [r0,#0x10]
	str	r1, [r0,#0x14]
	mov	r3, #3
	str	r3, [r6]
	orr	r1, r0, #0x00200000  @ R1 = TIMERBASE
	mov	r4, #0x440
	str	r4, [r1,#0xa0]
	mov	r4, #0xb
	str	r4, [r1,#0xb0]
	mvn	r4, r2
	str	r4, [r1,#0xa8]
	str	r3, [r1,#0xa4]
	orr	r3, r1, #0x00800000  @ R3 = GPIOBASE (temporary)
gpioloop:
	ldr	r4, [sp], #4
	str	r4, [r3], #0xc
	str	r2, [r3], #0x4
	str	r2, [r3], #0x10
	tst	r3, #0x200
	beq	gpioloop
	str	r7, [r3]
	add	r7, r7, #0x100
	str	r7, [r3]
	ldr	r7, [r3,#-0x30]
	bic	r7, r7, #0x40
	str	r7, [r3,#-0x30]
	ldr	r7, [r3,#-0x34]
	orr	r7, r7, #0x40
	str	r7, [r3,#-0x34]
	add	r5, r0, #0x00100000  @ R5 = I2CBASE
	bl	i2cwaitrdy           @ R3 = I2C address (temporary)
	mov	r7, #0x40            @ R4 = I2C data (temporary)
	str	r7, [r5,#8]          @ R6 = I2C scratch register (temporary)
	bl	i2cwaitrdy           @ R7 = I2C return address (temporary)
	str	r2, [r5,#0x14]       @ R0 = coldboot flag (temporary)
	bl	i2cwaitrdy
	str	r2, [r5,#0x18]
	bl	i2cwaitrdy
	mov	r6, #0x80
	str	r6, [r5,#0x4]
	bl	i2cwaitrdy
	str	r2, [r5]
	bl	i2cwaitrdy
	str	r2, [r5,#0x4]
	bl	i2cwaitrdy
	str	r7, [r5,#0xc]
	bl	i2cwaitrdy
	mov	r6, #0x184
	str	r6, [r5]
	bl	i2cwaitrdy
	mov	r6, #0x10
	str	r6, [r5,#0x4]
	bl	i2cwaitrdy
	mov	r3, #0x16
	mvn	r4, r2
	bl	pmutransfer
	and	r0, r4, #7
	mov	r3, #0xc
	mvn	r4, r2
	bl	pmutransfer
	and	r4, r4, #8
	orr	r0, r0, r4
pmuloop:
	ldrb	r3, [sp], #1
	ldrb	r4, [sp], #1
	cmp	r3, #0xff
	adrne	lr, pmuloop
	bne	pmutransfer
	mov	r3, #0x19
	mvn	r4, r2
	bl	pmutransfer
	tst	r4, #1
	beq	pwrok
	mov	r3, #0x4b
	mvn	r4, r2
	bl	pmutransfer
	tst	r4, #4
	bne	pwrok
	mov	r3, #0x12
	mvn	r4, r2
	bl	pmutransfer
	tst	r4, #4
	bne	pwrok
poweroff:
	mov	r3, #0xc
	mov	r4, #1
	bl	pmutransfer
	b	poweroff

values1:
	.word 0x00000FFF @ R4 - DMA maximum transfer size
	.word 0x00012C00 @ R5 - LCD framebuffer pixel count
	.word 0x22010000 @ R6 - LCD framebuffer address
	.word 0x38300040 @ R7 - LCD data port address
	.word 0x2203D000 @ R8 - LCD DMA linked list address
	.word 0x74240000 @ R9 - LCD DMA channel settings
	.word 0x0005107D @ R10 - CP15r1
	.word 0x2203C000 @ R11 - First level page table
	.word 0x00000C1E @ R12 - Default segment flags
	.word 0x3C500000 @ R0 - SYSCON base
	.word 0x2007CE65 @ R1 - PWRCON(0)
	.word 0x0003EFC9 @ R2 - PWRCON(1)
	.word 0x01002401 @ R3
	.word 0x00007E90 @ R4
	.word 0x00404101 @ R5
	.word 0x38501000 @ R6
	.word 0x00060400 @ R7
	.word 0x0000080D @ R8
	.word 0x0006105D @ R9
	.word 0x001FB621 @ R10
	.word 0x0790682B @ R11
	.word 0x00008040 @ R12

	.word 0x5322222F @ PCON0
	.word 0xEEEEEE00 @ PCON1
	.word 0x2332EEEE @ PCON2
	.word 0x3333E222 @ PCON3
	.word 0x33333333 @ PCON4
	.word 0x33333333 @ PCON5
	.word 0x3F000E33 @ PCON6
	.word 0xEEEEEEEE @ PCON7
	.word 0xEEEEEEEE @ PCON8
	.word 0xEEEEEEEE @ PCON9
	.word 0xE0EEEEEE @ PCONA
	.word 0xEF00EE0E @ PCONB
	.word 0xEEEE0EEF @ PCONC
	.word 0xEEEEEEEE @ PCOND
	.word 0x202222EE @ PCONE
	.word 0xEEEE0E22 @ PCONF

	.byte 0x0c, 0    @ PMU init address-data pairs
	.byte 0x2d, 0x15
	.byte 0x2e, 1
	.byte 0x37, 0x15
	.byte 0x38, 4
	.byte 0x2f, 0x15
	.byte 0x30, 1
	.byte 0x3b, 0
	.byte 0x3c, 0x8c
	.byte 0x13, 3
	.byte 0x14, 0
	.byte 0x15, 0
	.byte 0x22, 0x2f
	.byte 0x23, 1
	.byte 0x24, 0
	.byte 0x25, 0x10
	.byte 0x26, 9
	.byte 0x27, 1
	.byte 0x1a, 0x6f
	.byte 0x1b, 0
	.byte 0x1c, 0
	.byte 0x1d, 0x59
	.byte 0x33, 9
	.byte 0x34, 1
	.byte 0x39, 0x18
	.byte 0x3a, 0
	.byte 0x18, 0xa
	.byte 0x19, 4
	.byte 0x58, 0
	.byte 0x07, 0xb0
	.byte 0x08, 3
	.byte 0x09, 0xfe
	.byte 0x0a, 0xfc
	.byte 0x0b, 0xff
	.byte 0x86, 0xfd
	.byte 0x0d, 0xdf
	.byte 0x0e, 0xaa
	.byte 0x0f, 0x4a
	.byte 0x10, 5
	.byte 0x11, 0x27
	.byte 0x17, 1
	.byte 0x2a, 5
	.byte 0xff, 0xff @ PMU init terminator

lcddata_ref:
	.hword lcddata_0 - lcddata_ref
	.hword lcddata_1 - lcddata_ref - 2
	.hword lcddata_2 - lcddata_ref - 4
	.hword lcddata_3 - lcddata_ref - 6
lcddata_0:
	.hword 0x4011
	.hword 0xc78f
	.hword 0x8135
	.byte 0x00
	.hword 0x813a
	.byte 0x06
	.hword 0x8136
	.byte 0x00
	.hword 0x4013
	.hword 0x4029
	.hword 0xc000
lcddata_1:
	.hword 0x95b0
	.byte 0x3a
	.byte 0x3a
	.byte 0x80
	.byte 0x80
	.byte 0x0a
	.byte 0x0a
	.byte 0x0a
	.byte 0x0a
	.byte 0x0a
	.byte 0x0a
	.byte 0x0a
	.byte 0x0a
	.byte 0x3c
	.byte 0x30
	.byte 0x0f
	.byte 0x00
	.byte 0x01
	.byte 0x54
	.byte 0x06
	.byte 0x66
	.byte 0x66
	.hword 0x81b8
	.byte 0xd8
	.hword 0x9eb1
	.byte 0x14
	.byte 0x59
	.byte 0x00
	.byte 0x15
	.byte 0x57
	.byte 0x27
	.byte 0x04
	.byte 0x85
	.byte 0x14
	.byte 0x59
	.byte 0x00
	.byte 0x15
	.byte 0x57
	.byte 0x27
	.byte 0x04
	.byte 0x85
	.byte 0x14
	.byte 0x09
	.byte 0x15
	.byte 0x57
	.byte 0x27
	.byte 0x04
	.byte 0x05
	.byte 0x14
	.byte 0x09
	.byte 0x15
	.byte 0x57
	.byte 0x27
	.byte 0x04
	.byte 0x05
	.hword 0x40d2
	.hword 0x0101
	.hword 0x8de0
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x05
	.byte 0x0b
	.byte 0x12
	.byte 0x16
	.byte 0x1f
	.byte 0x25
	.byte 0x22
	.byte 0x24
	.byte 0x29
	.byte 0x1c
	.hword 0x8de1
	.byte 0x08
	.byte 0x01
	.byte 0x01
	.byte 0x06
	.byte 0x0b
	.byte 0x11
	.byte 0x15
	.byte 0x1f
	.byte 0x27
	.byte 0x26
	.byte 0x29
	.byte 0x2f
	.byte 0x1e
	.hword 0x8de2
	.byte 0x07
	.byte 0x01
	.byte 0x01
	.byte 0x05
	.byte 0x09
	.byte 0x0f
	.byte 0x13
	.byte 0x1e
	.byte 0x26
	.byte 0x25
	.byte 0x28
	.byte 0x2e
	.byte 0x1e
	.hword 0x8de3
	.byte 0x0d
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x05
	.byte 0x0b
	.byte 0x12
	.byte 0x16
	.byte 0x1f
	.byte 0x25
	.byte 0x22
	.byte 0x24
	.byte 0x29
	.byte 0x1c
	.hword 0x8de4
	.byte 0x08
	.byte 0x01
	.byte 0x01
	.byte 0x06
	.byte 0x0b
	.byte 0x11
	.byte 0x15
	.byte 0x1f
	.byte 0x27
	.byte 0x26
	.byte 0x29
	.byte 0x2f
	.byte 0x1e
	.hword 0x8de5
	.byte 0x07
	.byte 0x01
	.byte 0x01
	.byte 0x05
	.byte 0x09
	.byte 0x0f
	.byte 0x13
	.byte 0x1e
	.byte 0x26
	.byte 0x25
	.byte 0x28
	.byte 0x2e
	.byte 0x1e
	.hword 0x813a
	.byte 0x06
	.hword 0x81c2
	.byte 0x00
	.hword 0x8135
	.byte 0x00
	.hword 0x4011
	.hword 0xce88
	.hword 0x4013
	.hword 0x4029
	.hword 0xc000
lcddata_2:
lcddata_3:
	.hword 0xc010
	.hword 0x4008
	.hword 0x0808
	.hword 0x8110
	.byte 0x13
	.hword 0x4011
	.hword 0x0300
	.hword 0x4012
	.hword 0x0101
	.hword 0x4013
	.hword 0x0a03
	.hword 0x4014
	.hword 0x0a0e
	.hword 0x4015
	.hword 0x0a19
	.hword 0x4016
	.hword 0x2402
	.hword 0x8118
	.byte 0x01
	.hword 0x8190
	.byte 0x21
	.hword 0x4300
	.hword 0x0307
	.hword 0x4301
	.hword 0x0003
	.hword 0x4302
	.hword 0x0402
	.hword 0x4303
	.hword 0x0303
	.hword 0x4304
	.hword 0x0300
	.hword 0x4305
	.hword 0x0407
	.hword 0x4306
	.hword 0x1c04
	.hword 0x4307
	.hword 0x0307
	.hword 0x4308
	.hword 0x0003
	.hword 0x4309
	.hword 0x0402
	.hword 0x430a
	.hword 0x0303
	.hword 0x430b
	.hword 0x0300
	.hword 0x430c
	.hword 0x0407
	.hword 0x430d
	.hword 0x1c04
	.hword 0x4310
	.hword 0x0707
	.hword 0x4311
	.hword 0x0407
	.hword 0x4312
	.hword 0x0306
	.hword 0x4313
	.hword 0x0303
	.hword 0x4314
	.hword 0x0300
	.hword 0x4315
	.hword 0x0407
	.hword 0x4316
	.hword 0x1c01
	.hword 0x4317
	.hword 0x0707
	.hword 0x4318
	.hword 0x0407
	.hword 0x4319
	.hword 0x0306
	.hword 0x431a
	.hword 0x0303
	.hword 0x431b
	.hword 0x0300
	.hword 0x431c
	.hword 0x0407
	.hword 0x431d
	.hword 0x1c01
	.hword 0x4320
	.hword 0x0206
	.hword 0x4321
	.hword 0x0102
	.hword 0x4322
	.hword 0x0404
	.hword 0x4323
	.hword 0x0303
	.hword 0x4324
	.hword 0x0300
	.hword 0x4325
	.hword 0x0407
	.hword 0x4326
	.hword 0x1c1f
	.hword 0x4327
	.hword 0x0206
	.hword 0x4328
	.hword 0x0102
	.hword 0x4329
	.hword 0x0404
	.hword 0x432a
	.hword 0x0303
	.hword 0x432b
	.hword 0x0300
	.hword 0x432c
	.hword 0x0407
	.hword 0x432d
	.hword 0x1c1f
	.hword 0x4400
	.hword 0x001d
	.hword 0x4401
	.hword 0x0001
	.hword 0x4205
	.hword 0x0060
	.hword 0x8107
	.byte 0x01
	.hword 0x8131
	.byte 0x71
	.hword 0x4110
	.hword 0x0001
	.hword 0x4100
	.hword 0x17b0
	.hword 0x4101
	.hword 0x0220
	.hword 0x4102
	.hword 0x00bd
	.hword 0x4103
	.hword 0x1500
	.hword 0x4105
	.hword 0x0103
	.hword 0x4106
	.hword 0x0105
	.hword 0x8107
	.byte 0x21
	.hword 0x4102
	.hword 0x00bd
	.hword 0x4001
	.hword 0x0110
	.hword 0x4003
	.hword 0x0230
	.hword 0x4002
	.hword 0x0500
	.hword 0x8107
	.byte 0x31
	.hword 0x8130
	.byte 0x07
	.hword 0xc744
	.hword 0x4030
	.hword 0x03ff
	.hword 0xce88
	.hword 0x8107
	.byte 0x72
	.hword 0xe454
	.hword 0x4007
	.hword 0x0173
	.hword 0xc000

	.code 16

thumb_nrv2e_d8:
	mov r7,r2
        mov r4,#1; neg r5,r4 @ r5= -1 initial condition
        lsl r4,#31 @ 1<<31: refill next time
        mov r6,#5
        lsl r6,#8 @ 0x500 @ nrv2e M2_MAX_OFFSET
        b top_n2e

nrv2e_done:
	blx flushcache
	bx r7

get1_n2e: @ In: Carry set [from adding 0x80000000 (1<<31) to itself]
        ldrb r4,[r0] @ zero-extend next byte
        adc r4,r4 @ double and insert CarryIn as low bit
        add r0,#1
        lsl r4,#24 @ move to top byte, and set CarryOut from old bit 8
        mov pc,lr @ return, stay in current (THUMB) mode

lit_n2e:
        ldrb r3,[r0]; add r0,#1
        strb r3,[r2]; add r2,#1
top_n2e:
        add r4,r4; mov lr,pc; beq get1_n2e; bcs lit_n2e
        mov r1,#1; b getoff_n2e

off_n2e:
        sub r1,#1
        add r4,r4; mov lr,pc; beq get1_n2e; adc r1,r1
getoff_n2e:
        add r4,r4; mov lr,pc; beq get1_n2e; adc r1,r1
        add r4,r4; mov lr,pc; beq get1_n2e; bcc off_n2e

        sub r3,r1,#3 @ set Carry
        mov r1,#0 @ Carry unaffected
        blo offprev_n2e @ r1 was 2; tests Carry only
        lsl r3,#8
        ldrb r5,[r0]; add r0,#1 @ low 7+1 r4
        orr r5,r3
        mvn r5,r5; beq nrv2e_done @ r5= ~r5
        asr r5,#1; bcs lenlast_n2e
        b lenmore_n2e

offprev_n2e:
        add r4,r4; mov lr,pc; beq get1_n2e; bcs lenlast_n2e
lenmore_n2e:
        mov r1,#1
        add r4,r4; mov lr,pc; beq get1_n2e; bcs lenlast_n2e
len_n2e:
        add r4,r4; mov lr,pc; beq get1_n2e; adc r1,r1
        add r4,r4; mov lr,pc; beq get1_n2e; bcc len_n2e
        add r1,#6-2
        b gotlen_n2e

lenlast_n2e:
        add r4,r4; mov lr,pc; beq get1_n2e; adc r1,r1 @ 0,1,2,3
        add r1,#2
gotlen_n2e: @ 'cmn': add the inputs, set condition codes, discard the sum
        cmn r6,r5; bcs near_n2e @ within M2_MAX_OFFSET
        add r1,#1 @ too far away, so minimum match length is 3
near_n2e:
        ldrb r3,[r2] @ force cacheline allocate
copy_n2e:
        ldrb r3,[r2,r5]
        strb r3,[r2]; add r2,#1
        sub r1,#1; bne copy_n2e
        b top_n2e

	.code 32

font:
	.byte 0, 0, 0, 0, 0
	.byte 0, 0, 95, 0, 0
	.byte 0, 7, 0, 7, 0
	.byte 20, 127, 20, 127, 20
	.byte 36, 42, 127, 42, 18
	.byte 35, 19, 8, 100, 98
	.byte 54, 73, 85, 34, 80
	.byte 5, 3, 0, 0, 0
	.byte 28, 34, 65, 0, 0
	.byte 0, 0, 65, 34, 28
	.byte 20, 8, 62, 8, 20
	.byte 8, 8, 62, 8, 8
	.byte 0, -96, 96, 0, 0
	.byte 8, 8, 8, 8, 8
	.byte 0, 96, 96, 0, 0
	.byte 32, 16, 8, 4, 2
	.byte 62, 81, 73, 69, 62
	.byte 0, 66, 127, 64, 0
	.byte 66, 97, 81, 73, 70
	.byte 33, 65, 69, 75, 49
	.byte 24, 20, 18, 127, 16
	.byte 39, 69, 69, 69, 57
	.byte 60, 74, 73, 73, 48
	.byte 1, 113, 9, 5, 3
	.byte 54, 73, 73, 73, 54
	.byte 6, 73, 73, 41, 30
	.byte 0, 54, 54, 0, 0
	.byte 0, 86, 54, 0, 0
	.byte 8, 20, 34, 65, 0
	.byte 20, 20, 20, 20, 20
	.byte 0, 65, 34, 20, 8
	.byte 2, 1, 81, 9, 6
	.byte 50, 73, 121, 65, 62
	.byte 124, 18, 17, 18, 124
	.byte 127, 73, 73, 73, 62
	.byte 62, 65, 65, 65, 34
	.byte 127, 65, 65, 34, 28
	.byte 127, 73, 73, 73, 65
	.byte 127, 9, 9, 9, 1
	.byte 62, 65, 73, 73, 58
	.byte 127, 8, 8, 8, 127
	.byte 0, 65, 127, 65, 0
	.byte 32, 64, 65, 63, 1
	.byte 127, 8, 20, 34, 65
	.byte 127, 64, 64, 64, 64
	.byte 127, 2, 12, 2, 127
	.byte 127, 4, 8, 16, 127
	.byte 62, 65, 65, 65, 62
	.byte 127, 9, 9, 9, 6
	.byte 62, 65, 81, 33, 94
	.byte 127, 9, 25, 41, 70
	.byte 38, 73, 73, 73, 50
	.byte 1, 1, 127, 1, 1
	.byte 63, 64, 64, 64, 63
	.byte 31, 32, 64, 32, 31
	.byte 127, 32, 24, 32, 127
	.byte 99, 20, 8, 20, 99
	.byte 3, 4, 120, 4, 3
	.byte 97, 81, 73, 69, 67
	.byte 0, 127, 65, 65, 0
	.byte 2, 4, 8, 16, 32
	.byte 0, 65, 65, 127, 0
	.byte 4, 2, 1, 2, 4
	.byte 64, 64, 64, 64, 64
	.byte 1, 2, 4, 0, 0
	.byte 32, 84, 84, 84, 120
	.byte 127, 68, 68, 68, 56
	.byte 56, 68, 68, 68, 40
	.byte 56, 68, 68, 68, 127
	.byte 56, 84, 84, 84, 24
	.byte 8, 126, 9, 1, 2
	.byte 8, 84, 84, 84, 60
	.byte 127, 4, 4, 4, 120
	.byte 0, 68, 125, 64, 0
	.byte 32, 64, 64, 61, 0
	.byte 127, 16, 40, 68, 0
	.byte 0, 65, 127, 64, 0
	.byte 124, 4, 24, 4, 120
	.byte 124, 8, 4, 4, 120
	.byte 56, 68, 68, 68, 56
	.byte 124, 20, 20, 20, 24
	.byte 8, 20, 20, 20, 124
	.byte 124, 8, 4, 4, 8
	.byte 72, 84, 84, 84, 32
	.byte 4, 63, 68, 64, 32
	.byte 60, 64, 64, 32, 124
	.byte 28, 32, 64, 32, 28
	.byte 60, 64, 56, 64, 60
	.byte 68, 40, 16, 40, 68
	.byte 12, 80, 80, 80, 60
	.byte 68, 100, 84, 76, 68
	.byte 0, 8, 54, 65, 0
	.byte 0, 0, 119, 0, 0
	.byte 0, 65, 54, 8, 0
	.byte 2, 1, 2, 4, 2

	.align 2

foundfile:
	ldr	r12, [r11,#0xc]
	ldr	r11, [r11,#0x8]
	tst	r12, #0x80000000
	bic	r12, r12, #0xff000000
	bic	r12, r12, #0xf00000
	bne	compressed
    	mov	r1, #0x08000000
	bl	bootflash_read
	mov	lr, #0x08000000
	b	flushcache
compressed:
    	mov	r1, #0x09000000
	bl	bootflash_read
	mov	r0, #0x09000000
	mov	r2, #0x08000000
	blx	thumb_nrv2e_d8


rendertext:
	ldrb	r3, [r0], #1
	cmp	r3, #0
	moveq	pc, lr
	adr	r6, font
	sub	r3, r3, #0x20
	cmp	r3, #0x5f
	addcc	r6, r3,lsl#2
	addcc	r6, r3
	mov	r3, #5
rendertext_col:
	mov	r7, r1
	ldrb	r9, [r6], #1
rendertext_row:
	tst	r9, #1
	strneh	r2, [r7]
	add	r7, r7, #640
	movs	r9, r9,lsr#1
	bne	rendertext_row
	add	r1, r1, #2
	subs	r3, r3, #1
	bne	rendertext_col
	add	r1, r1, #2
	b	rendertext

spisend:
	mov	r6, #1
	str	r6, [r7,#0x34]
spisend_wait:
	ldr	r6, [r7,#0x8]
	and	r6, r6, #0x1f0
	cmp	r6, #0x100
	beq	spisend_wait
	str	r0, [r7,#0x10]
spisend_wait2:
	ldr	r6, [r7,#0x8]
	ands	r6, r6, #0x3e00
	beq	spisend_wait2
	ldr	r0, [r7,#0x20]
	mov	pc, lr
    	
spirecv:
	str	r0, [r7,#0x34]
	ldr	r6, [r7,#0x4]
	orr	r6, r6, #1
	str	r6, [r7,#0x4]
spirecv_loop:
	ldr	r6, [r7,#0x8]
	ands	r6, r6, #0x3e00
	beq	spirecv_loop
	ldr	r6, [r7,#0x20]
	strb	r6, [r1], #1
	subs	r0, r0, #1
	bne	spirecv_loop
	ldr	r6, [r7,#0x4]
	bic	r6, r6, #1
	str	r6, [r7,#0x4]
	mov	pc, lr

bootflash_read:
	mov	sp, lr
bootflash_wait:
	ldr	r0, [r3,#0x4]
	bic	r0, r0, #1
	str	r0, [r3,#0x4]
	mov	r0, #5
	bl	spisend
	mov	r0, #0xff
	bl	spisend
	tst	r0, #1
	ldr	r0, [r3,#0x4]
	orr	r0, r0, #1
	str	r0, [r3,#0x4]
	bne	bootflash_wait
	ldr	r0, [r3,#0x4]
	bic	r0, r0, #1
	str	r0, [r3,#0x4]
	mov	r0, #3
	bl	spisend
	mov	r0, r11,lsr#16
	bl	spisend
	mov	r0, r11,lsr#8
	and	r0, r0, #0xff
	bl	spisend
	and	r0, r11, #0xff
	bl	spisend
	mov	r0, r12
	bl	spirecv
	ldr	r0, [r3,#0x4]
	orr	r0, r0, #1
	str	r0, [r3,#0x4]
	mov	pc, sp

lcdbyteseq:
	mov	r9, r0,lsr#8
	and	r0, r0, #0xff
	bl	sendlcdc
lcdbyteseqloop:
	ldrb	r0, [sp], #1
	bl	sendlcdd
	subs	r9, r9, #1
	bne	lcdbyteseqloop
	b	lcdloop

sendlcdc:
	ldr	r6, [r4,#0x1c]
	ands	r6, r6, #0x10
	bne	sendlcdc
	str	r0, [r4,#0x04]
	mov	pc, lr

sendlcdd:
	ldr	r6, [r4,#0x1c]
	ands	r6, r6, #0x10
	bne	sendlcdd
	str	r0, [r4,#0x40]
	mov	pc, lr

i2cwaitrdy:
	ldr	r6, [r5,#0x10]
	cmp	r6, #0
	bne	i2cwaitrdy
	mov	pc, lr

i2cwait:
	ldr	r6, [r5]
	tst	r6, #0x10
	beq	i2cwait
	mov	pc, lr

pmutransfer:
	mov	r7, lr
	mov	lr, #0xb7
	str	lr, [r5]
	mov	lr, #0xe6
	str	lr, [r5,#0xc]
	mov	lr, #0xf0
	str	lr, [r5,#0x4]
	bl	i2cwait
	str	r3, [r5,#0xc]
	mov	lr, #0xb7
	str	lr, [r5]
	bl	i2cwait
	cmn	r4, #1
	beq	pmuskip
	str	r4, [r5,#0xc]
	mov	lr, #0xb7
	str	lr, [r5]
	bl	i2cwait
	mov	lr, #0xd0
	b	pmudone
pmuskip:
	mov	lr, #0xe7
	str	lr, [r5,#0xc]
	mov	lr, #0xb0
	str	lr, [r5,#0x4]
	mov	lr, #0xb7
	str	lr, [r5]
	bl	i2cwait
	mov	lr, #0x37
	str	lr, [r5]
	bl	i2cwait
	ldr	r3, [r5,#0xc]
	mov	lr, #0x90
pmudone:
	str	lr, [r5,#0x4]
	mov	lr, #0xb7
	str	lr, [r5]
pmuwait:
	ldr	lr, [r5,#0x4]
	tst	lr, #0x20
	bne	pmuwait
	mov	pc, r7

udelay:
	ldr	r6, [r1,#0xb4]
	add	r0, r0, r6
udelayloop:
	ldr	r6, [r1,#0xb4]
	cmp	r6, r0
	bmi	udelayloop
	mov	pc, lr


pwrok:
	sub	r7, r5, #0x04500000  @ R7 = MIUBASE (temporary)
	cmp	r0, #0
	moveq	r6, #0x11
	streq	r6, [r7]
	str	r8, [r7]
	str	r2, [r7,#0xf0]
	str	r9, [r7,#0x8]
	str	r10, [r7,#0x10]
	mov	r8, #0x1800
	orr	r6, r8, #0x45
	str	r6, [r7,#0x200]
	str	r6, [r7,#0x204]
	str	r8, [r7,#0x210]
	str	r8, [r7,#0x214]
	str	r6, [r7,#0x220]
	str	r6, [r7,#0x224]
	add	r6, r6, #0x40
	str	r6, [r7,#0x230]
	str	r6, [r7,#0x234]
	mov	r6, #0x19
	str	r6, [r7,#0x14]
	str	r6, [r7,#0x18]
	str	r11, [r7,#0x1c]
	ldr	r6, [r7,#0x314]
	bic	r6, r6, #0x10
	str	r6, [r7,#0x314]
	add	r8, r7, #0x2c
	mov	r9, #0x24
sdramloop:
	ldr	r6, [r8]
	bic	r6, r6, #0x01000000
	str	r6, [r8], #4
	subs	r9, r9, #1
	bne	sdramloop
	mov	r6, #0x540
	str	r6, [r7,#0x1cc]
	ldr	r6, [r7,#0x1d4]
	orr	r6, r6, #0x80
	str	r6, [r7,#0x1d4]
	mov	r6, #0x33
	str	r6, [r7,#0x4]
	str	r6, [r7,#0x4]
	orr	r8, r6, #0x200
	str	r8, [r7,#0x4]
	str	r6, [r7,#0x4]
	str	r6, [r7,#0x4]
	str	r6, [r7,#0x4]
	orr	r8, r8, #0x100
	str	r8, [r7,#0x4]
	str	r6, [r7,#0x4]
	str	r6, [r7,#0x4]
	str	r6, [r7,#0x4]
	str	r8, [r7,#0x4]
	str	r6, [r7,#0x4]
	str	r6, [r7,#0x4]
	str	r6, [r7,#0x4]
	cmp	r0, #0
	strne	r6, [r7,#0xc]
	bicne	r8, r8, #0x200
	strne	r8, [r7,#0x4]
	strne	r6, [r7,#0x4]
	strne	r6, [r7,#0x4]
	strne	r6, [r7,#0x4]
	strne	r12, [r7,#0xc]
	strne	r8, [r7,#0x4]
	strne	r6, [r7,#0x4]
	strne	r6, [r7,#0x4]
	strne	r6, [r7,#0x4]
	ldr	r6, [r7,#0x8]
	orr	r6, r6, #0x61000
	str	r6, [r7,#0x8]
	orr	r3, r1, #0x00800000  @ R3 = GPIOBASE
	sub	r4, r1, #0x04400000  @ R4 = LCDBASE
	ldr	r0, [r3,#0xc4]
	and	r8, r0, #0x30        @ R8 = LCD type GPIO bits << 4
	mov	r6, #0x80000000
	add	sp, sp, r8,lsr#3
	tst	r8, #0x20
	ldrh	r0, [sp]
	orrne	r6, r6, #0x188
	add	sp, sp, r0
	orr	r6, r6, #0xc20
	mov	r0, #0x33
	str	r6, [r4]
	str	r0, [r4,#0x20]
lcdloop:
	adr	lr, lcdloop
	ldrb	r6, [sp], #1
	ldrb	r7, [sp], #1
	orr	r6, r6, r7,lsl#8
	mov	r7, r6,lsr#14
	bics	r0, r6, #0xc000
	add	pc, pc, r7,lsl#2
val_80100db0:
	.word 0x80100db0
	b	sendlcdd
	b	sendlcdc
	bne	lcdbyteseq
	movne	r0, r0,lsl#4
	bne	udelay
	mov	r10, #0x22000000
	ldr	r6, val_80100db0
	orr	r10, r10, #0x10000
	str	r6, [r4]
	add	r11, r10, #0x25800
	mov	r0, #-1
fillbuff:
	str	r0, [r11,#-4]!
	cmp	r10, r11
	bne	fillbuff
	mov	r1, r11
	adr	r0, text
	bl	rendertext
	add	r1, r11, #0x2800
printerror:
	bl	rendertext
	bl	flushcache
	tst	r8, #0x20
	beq	leadisprotocol
	mov	r9, #0x200
	add	r0, r9, #0x10
	bl	sendlcdc
	mov	r0, #0
	bl	sendlcdd
	add	r0, r9, #0x11
	bl	sendlcdc
	add	r0, r9, #0x3f
	bl	sendlcdd
	add	r0, r9, #0x12
	bl	sendlcdc
	mov	r0, #0
	bl	sendlcdd
	add	r0, r9, #0x13
	bl	sendlcdc
	mov	r0, #0xef
	bl	sendlcdd
	mov	r0, r9
	bl	sendlcdc
	mov	r0, #0
	bl	sendlcdd
	add	r0, r9, #1
	bl	sendlcdc
	mov	r0, #0
	bl	sendlcdd
	add	r0, r9, #2
	bl	sendlcdc
	b	blit
leadisprotocol:
	mov	r0, #0x2a
	bl	sendlcdc
	mov	r0, #0
	bl	sendlcdd
	mov	r0, #0
	bl	sendlcdd
	mov	r0, #1
	bl	sendlcdd
	mov	r0, #0x3f
	bl	sendlcdd
	mov	r0, #0x2b
	bl	sendlcdc
	mov	r0, #0
	bl	sendlcdd
	mov	r0, #0
	bl	sendlcdd
	mov	r0, #0
	bl	sendlcdd
	mov	r0, #0xef
	bl	sendlcdd
	mov	r0, #0x2c
	bl	sendlcdc
blit:
	sub	r7, r4, #0x00100000  @ R7 = DMA0BASE (temporary)
	mov	r0, #1
	str	r0, [r7,#0x30]
	mov	r0, #0x22000000
	orr	r0, r0, #0x3d000
	add	r1, r7, #0x100
	mov	r13, #0x8800
	orr	r13, r13, #0xc1
	ldmia	r0, {r9-r12}
	stmia	r1, {r9-r13}
	tst	r5, #0xff
failed:
	bne	failed
	mov	r3, #0x2b
	mov	r4, #4
	bl	pmutransfer
	mov	r3, #0x28
	mov	r4, #14
	bl	pmutransfer
	mov	r3, #0x29
	mov	r4, #1
	bl	pmutransfer
	sub	r7, r5, #0x00300000  @ R7 = SPIBASE
	orr	r3, r5, #0x00900000  @ R3 = GPIOBASE
	mov	r0, #0xf
	str	r0, [r7,#0x8]
	ldr	r0, [r7]
	orr	r0, r0, #0xc
	str	r0, [r7]
	mov	r0, #4
	str	r0, [r7,#0x30]
	mov	r0, #6
	str	r0, [r7,#0xc]
	ldr	r0, val_00010618
	str	r0, [r7,#0x4]
	ldr	r0, [r7]
	orr	r0, r0, #0xc
	str	r0, [r7]
	mov	r0, #1
	str	r0, [r7]
    	sub	r1, r9, #0x1000
	mov	r11, #0
	mov	r12, #0x1000
	bl	bootflash_read
    	sub	r11, r9, #0x1000
	ldrd	r4, bootfilename
searchfile:
	ldrd	r12, [r11]
	cmp	r4, r12
	cmpeq	r5, r13
	beq	foundfile
	add	r11, r11, #0x10
	tst	r11, #0x10000
	beq	searchfile
	adr	r0, errormessage
	sub	r4, r3, #0x04c00000  @ R4 = LCDBASE
	mov	r1, #0x22000000
	orr	r1, r1, #0x13c00
	b	printerror

val_00010618:
	.word 0x00010618

bootfilename:
	.ascii "emcore  "

text:
	.ascii "emCORE Loader v"
	.ascii VERSION
	.ascii " r"
	.ascii VERSION_SVN
	.ascii "\0"
	.ascii "Loading emCORE...\0"

errormessage:
	.ascii "File not found!\0"
