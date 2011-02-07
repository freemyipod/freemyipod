@
@
@    emBIOS Loader for iPod Classic
@
@    Copyright 2010 TheSeven
@
@
@    This file is part of emBIOS.
@
@    emBIOS is free software: you can redistribute it and/or
@    modify it under the terms of the GNU General Public License as
@    published by the Free Software Foundation, either version 2 of the
@    License, or (at your option) any later version.
@
@    emBIOS is distributed in the hope that it will be useful,
@    but WITHOUT ANY WARRANTY; without even the implied warranty of
@    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
@    See the GNU General Public License for more details.
@
@    You should have received a copy of the GNU General Public License along
@    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
@
@

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
	ldmia	sp!, {r10-r12}
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
	.word 0x0005107D @ R10 - CP15r1
	.word 0x2203C000 @ R11 - First level page table
	.word 0x00000C1E @ R12 - Default segment flags
	.word 0x3C500000 @ R0 - SYSCON base
	.word 0xFDFFFFE5 @ R1 - PWRCON(0)
	.word 0xFFFFEFCD @ R2 - PWRCON(1)
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

	.align 2

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
	adr	r1, _stubend + 4
	ldr	r6, val_80100db0
	ldr	r0, _stubend
	str	r6, [r4]
	add	r0, r1, r0
	mov	r2, #0x08000000
movepayloadloop:
	cmp	r0, r1
	ldrhi	r3, [r1], #4
	strhi	r3, [r2], #4
	bhi	movepayloadloop
	bl	flushcache
	mcr	p15, 0, r0,c7,c5
	mov	pc, #0x08000000

_stubend: