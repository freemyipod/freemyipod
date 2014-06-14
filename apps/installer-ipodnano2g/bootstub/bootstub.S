@
@
@    emCORE Loader for iPod Nano 2G
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

.global _start
_start:

	msr	cpsr_c, #0xd3
	mrc	p15, 0, r0,c1,c0
	bic	r0, r0, #1
	mcr	p15, 0, r0,c1,c0
	mov	r0, #0
	mcr	p15, 0, r0,c7,c5
	adr	lr, cacheflush_done

flushcache:
	mov	r0, #0
flushcache_loop:
	mcr	p15, 0, r0,c7,c14,2
	add	r10, r0, #0x10
	mcr	p15, 0, r10,c7,c14,2
	add	r10, r10, #0x10
	mcr	p15, 0, r10,c7,c14,2
	add	r10, r10, #0x10
	mcr	p15, 0, r10,c7,c14,2
	adds	r0, r0, #0x04000000
	bne	flushcache_loop
	mcr	p15, 0, r0,c7,c10,4
	mov	pc, lr

cacheflush_done:
	adr	r0, _start
	adr	r1, _stubend + 4
	mov	r2, #0x22000000
copyloop:
	cmp	r1, r0
	ldrhi	r3, [r0], #4
	strhi	r3, [r2], #4
	bhi	copyloop
	str	r1, [r2]
	mov	r0, #0x22000000
	add	pc, r0, #sramstart - _start
sramstart:

	adr	lr, values1
	ldmia	lr!, {r0-r13}
	str	r1, [r0,#0x88]
	str	r2, [r0,#0xf0]
	str	r4, [r3,#0x38]
	mov	r1, #0
	str	r7, [r0]
	str	r1, [r0,#0x04]
	str	r8, [r0,#0x10]
	str	r1, [r0,#0x14]
	str	r9, [r0,#0x20]
	str	r1, [r0,#0x24]
	str	r10, [r0,#0x30]
	str	r1, [r0,#0x34]
	str	r9, [r0,#0x40]
	str	r1, [r0,#0x44]
	str	r11, [r0,#0x50]
	str	r1, [r0,#0x54]
	str	r12, [r0,#0x60]
	str	r1, [r0,#0x64]
	str	r13, [r0,#0xa0]
	str	r1, [r0,#0xa4]
	str	r5, [r0,#0xb0]
	mov	r3, #1
	str	r3, [r0,#0xb4]
	str	r6, [r0,#0xc0]
	ldmia	lr, {r4-r14}
	str	r1, [r0,#0xc4]
	str	r6, [r0,#0xd0]
	str	r1, [r0,#0xd4]
	str	r6, [r0,#0xe0]
	str	r1, [r0,#0xe4]
	str	r7, [r0,#0xf0]
	str	r1, [r0,#0xf4]
	ldr	r3, [r8,#0x24]
	orr	r3, r3, #3
	str	r3, [r8,#0x24]
	ldr	r3, [r8,#0x28]
	bic	r3, r3, #0x4000
	str	r3, [r8,#0x28]
	mov	r3, #0x3C400000
	str	r1, [r3]
	str	r12, [r8,#0x0c]
	ldr	r3, [r8,#0x24]
	orr	r3, r3, #0x54
	str	r3, [r8,#0x24]
	mov	r3, #0x2000
waitpll1:
	subs	r3, r3, #1
	bne	waitpll1
	ldr	r3, [r8]
	bic	r3, r3, r5
	orr	r3, r3, r4
	str	r3, [r8]
	ldr	r3, [r8,#0x24]
	bic	r3, r3, #1
	str	r3, [r8,#0x24]
	mov	r3, #0x84
	str	r3, [r0,#0x68]
	str	r13, [r8,#0x04]
	mov	r3, #0xa000
waitpll2:
	subs	r3, r3, #1
	bne	waitpll2
	ldr	r3, [r8,#0x24]
	orr	r3, r3, #1
	str	r3, [r8,#0x24]
	mov	r3, #0x6000
waitpll3:
	subs	r3, r3, #1
	bne	waitpll3
	ldr	r3, [r8]
	bic	r3, r3, r9
	orr	r3, r3, r10
	str	r3, [r8]
	ldr	r3, [r8,#0x3c]
	bic	r3, r3, #0x400
	orr	r3, r3, #0x280
	str	r3, [r8,#0x3c]
	ldr	r3, [r8,#0x10]
	bic	r3, r3, #0x1f8
	orr	r3, r3, #0x37
	str	r3, [r8,#0x10]
	mrc     p15, 0, r3,c1,c0
	orr     r3, r3, #0xc0000000
	mcr     p15, 0, r3,c1,c0
	str	r11, [r8,#0x28]
	str	r14, [r8,#0x40]
	sub	r6, r0, #0x00800000
	mov	r5, #0xf700
	str	r5, [r6,#0x88]
	mov	r5, #0x1100
	orr	r5, r5, #0x11
	str	r5, [r0,#0x88]
	add	r4, r6, #0x00200000
	str	r1, [r4,#0x08]
	mov	r5, #0xf3
	str	r5, [r4]
	mov	r5, #0x10
	str	r5, [r4,#0x04]
	b	setuppmu

pmuinitdata:
	.byte 3, 0x39, 0x18, 0, 0x8c
	.byte 9, 0x13, 3, 0, 0, 7, 0, 5, 2, 0x6b, 0
	.byte 4, 0x1e, 0x12, 1, 0, 0x10
	.byte 6, 0x22, 0x2f, 1, 0, 0x10, 9, 1
	.byte 6, 0x31, 0x15, 1, 0x0f, 3, 0x15, 0
	.byte 1, 0x43, 0
	.byte 1, 0x4e, 0
	.byte 1, 0x58, 0
	.byte 5, 0x07, 0xb0, 0x0f, 0xfe, 0xfc, 0xff
	.byte 5, 0x0d, 0xdf, 0xaa, 0x4a, 5, 0x27
	.byte 0
	.align 2

val_006a49a5:
	.word 0x006a49a5

values1:
	.word 0x3CF00000 @ R0
	.word 0x00001111 @ R1
	.word 0x00000221 @ R2
	.word 0x38200000 @ R3
val_00990999:
	.word 0x00990999 @ R4
	.word 0x41001111 @ R5
	.word 0x11114444 @ R6
	.word 0x11221111 @ R7
	.word 0x11001744 @ R8
	.word 0x33333333 @ R9
	.word 0x11113333 @ R10
	.word 0x77721151 @ R11
	.word 0x12111111 @ R12
	.word 0x11111000 @ R13
	.word 0x60007000 @ R4
	.word 0x7F007F00 @ R5
	.word 0x44444444 @ R6
	.word 0x22222221 @ R7
	.word 0x3C500000 @ R8
	.word 0xFF00FF00 @ R9
	.word 0x20803180 @ R10
	.word 0xFFDFF7CF @ R11
	.word 0x00061800 @ R12
	.word 0x00021200 @ R13
	.word 0xFFFFEF7E @ R14

setuppmu:
	mov	sp, pc
	bl	getpowerok
	beq	poweroff
	adr	r2, pmuinitdata
pmuinitloop:
	ldrb	r3, [r2], #1
	movs	r3, r3
	beq	pmuinitdone
	ldrb	r1, [r2], #1
	mov	r0, #0xe6
	bl	i2csend
	b	pmuinitloop
pmuinitdone:
	orr	r12, r12, #0x00600000
	mov	r1, #1
	ldr	r0, [r12,#0x100]
	bic	r0, r0, #0x0f
	orr	r0, r0,	#1
	str	r0, [r12,#0x100]
	str	r1, [r12,#0x10c]
	sub	r11, r12, #0x04d00000
	ldr	r0, [r11,#0x80]
	bic	r0, r0, #0x20000
	str	r0, [r11,#0x80]
	ldr	r0, [r11]
	bic	r0, r0, #0xc00
	orr	r0, r0, #0x500
	str	r0, [r11]
	mov	r0, #3
	str	r0, [r11,#0x04]
	mov	r0, #1
	bl	sleepms
	mov	r0, #0x200
	orr	r0, r0, #0xed
	str	r0, [r11,#0x08]
	ldr	r0, val_006a49a5
	str	r0, [r11,#0x10]
	mov	r0, #0x37
	str	r0, [r11,#0x0c]
	ldr	r1, [r11,#0x04]
	bic	r1, r1, #0x700
	orr	r0, r1, #0x200
	str	r0, [r11,#0x04]
	orr	r0, r1, #0x300
	str	r0, [r11,#0x04]
	str	r0, [r11,#0x04]
	orr	r0, r1, #0x100
	str	r0, [r11,#0x04]
	orr	r0, r1, #0x500
	str	r0, [r11,#0x04]
	ldr	r0, [r11,#0x08]
	orr	r0, r0, #0x1000
	str	r0, [r11,#0x08]
	ldr	r0, val_00990999
	str	r0, [r11,#0x38]
	ldr	r0, [r12,#0xf0]
	bic	r0, r0, #0xff000000
	bic	r0, r0, #0x00ff0000
	orr	r0, r0,	#0x22000000
	orr	r0, r0,	#0x00220000
	str	r0, [r12,#0xf0]
	mov	r0, #0xf0
	str	r0, [r12,#0xfc]
	sub	r10, r12, #0x00d00000
	ldr	r0, [r10]
	mov	r0, r0,lsr#20
	mov	r0, r0,lsl#20
	str	r0, [r10]
	mov	r1, #0x3a000
	orr	r1, r1, #0x980
	str	r1, [r10,#0x08]
	orr	r0, r0, #0x300000
	str	r0, [r10]
	bl	initlcd
	mov	r0, #0x3f
	mcr	p15, 0, r0,c6,c0,1   @ CS0: 4GB at offset 0 - everything
	mcr	p15, 0, r0,c6,c0,0   @ DS0: 4GB at offset 0 - everything
	mov	r0, #0x31
	mcr	p15, 0, r0,c6,c1,1   @ CS1: SRAM/SDRAM mirror
	mcr	p15, 0, r0,c6,c1,0   @ DS1: SRAM/SDRAM mirror
	add	r0, r0, #0x08000000
	mcr	p15, 0, r0,c6,c2,1   @ CS2: SDRAM
	mcr	p15, 0, r0,c6,c2,0   @ DS2: SDRAM
	mov	r0, #0x23
	add	r0, r0, #0x22000000
	mcr	p15, 0, r0,c6,c3,1   @ CS3: SRAM
	mcr	p15, 0, r0,c6,c3,0   @ DS3: SRAM
	mov	r0, #0x27
	add	r0, r0, #0x24000000
	mcr	p15, 0, r0,c6,c4,1   @ CS4: NOR flash
	mcr	p15, 0, r0,c6,c4,0   @ DS4: NOR flash
	mcr	p15, 0, r1,c6,c5,1   @ CS5: unused
	mcr	p15, 0, r1,c6,c5,0   @ DS5: unused
	mcr	p15, 0, r1,c6,c6,1   @ CS6: unused
	mcr	p15, 0, r1,c6,c6,0   @ DS6: unused
	mcr	p15, 0, r1,c6,c7,1   @ CS7: unused
	mcr	p15, 0, r1,c6,c7,0   @ DS7: unused
	mov	r0, #0x1e
	mcr	p15, 0, r0,c2,c0, 1  @ CS1-4: cacheable
	mcr	p15, 0, r0,c2,c0, 0  @ DS1-4: cacheable
	mcr	p15, 0, r0,c3,c0, 0  @ DS1-4: write cacheable
	mov	r0, #0x300
	add	r0, r0, #0xff
	mcr	p15, 0, r0,c5,c0, 1  @ CS0-4: full access
	mcr	p15, 0, r0,c5,c0, 0  @ DS0-4: full access
	mrc	p15, 0, r0,c1,c0
	orr	r0, r0, #5
	orr	r0, r0, #0x1000
	mcr	p15, 0, r0,c1,c0     @ Re-enable the Protection Unit and caches
	mov	r0, #0xe6
	mov	r1, #0x2a
	mov	r2, #6
	bl	i2csendbyte
	mov	r0, #0xe6
	mov	r1, #0x2b
	mov	r2, #4
	bl	i2csendbyte
	mov	r0, #0xe6
	mov	r1, #0x28
	mov	r2, #10
	bl	i2csendbyte
	mov	r0, #0xe6
	mov	r1, #0x29
	mov	r2, #1
	bl	i2csendbyte
	ldr	r1, _stubend + 4
	ldr	r0, _stubend
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


lcd_inittab_type2:
	.byte 5, 10
	.byte 3, 0xa4
	.byte 4, 0x01
	.byte 5, 150
	.byte 3, 0x01
	.byte 2, 0x01
	.byte 2, 0x00
	.byte 3, 0x02
	.byte 2, 0x03
	.byte 2, 0x00
	.byte 3, 0x03
	.byte 2, 0x12
	.byte 2, 0x30
	.byte 3, 0x08
	.byte 2, 0x04
	.byte 2, 0x04
	.byte 3, 0x0e
	.byte 4, 0x10
	.byte 3, 0x70
	.byte 2, 0x10
	.byte 2, 0x00
	.byte 3, 0x71
	.byte 4, 0x01
	.byte 3, 0x30
	.byte 4, 0x02
	.byte 3, 0x31
	.byte 2, 0x04
	.byte 2, 0x00
	.byte 3, 0x32
	.byte 4, 0x07
	.byte 3, 0x33
	.byte 2, 0x05
	.byte 2, 0x00
	.byte 3, 0x34
	.byte 4, 0x07
	.byte 3, 0x35
	.byte 2, 0x07
	.byte 2, 0x03
	.byte 3, 0x36
	.byte 2, 0x05
	.byte 2, 0x07
	.byte 3, 0x37
	.byte 4, 0x05
	.byte 3, 0x38
	.byte 2, 0x14
	.byte 2, 0x04
	.byte 3, 0x39
	.byte 4, 0x0e
	.byte 3, 0x40
	.byte 2, 0x02
	.byte 2, 0x02
	.byte 3, 0x41
	.byte 4, 0x03
	.byte 3, 0x42
	.byte 4, 0x00
	.byte 3, 0x43
	.byte 2, 0x02
	.byte 2, 0x00
	.byte 3, 0x44
	.byte 2, 0x07
	.byte 2, 0x07
	.byte 3, 0x45
	.byte 2, 0x04
	.byte 2, 0x07
	.byte 3, 0x46
	.byte 2, 0x05
	.byte 2, 0x05
	.byte 3, 0x47
	.byte 4, 0x02
	.byte 3, 0x48
	.byte 4, 0x04
	.byte 3, 0x49
	.byte 4, 0x04
	.byte 3, 0x60
	.byte 2, 0x02
	.byte 2, 0x02
	.byte 3, 0x61
	.byte 4, 0x03
	.byte 3, 0x62
	.byte 4, 0x00
	.byte 3, 0x63
	.byte 2, 0x02
	.byte 2, 0x00
	.byte 3, 0x64
	.byte 2, 0x07
	.byte 2, 0x07
	.byte 3, 0x65
	.byte 2, 0x04
	.byte 2, 0x07
	.byte 3, 0x66
	.byte 2, 0x05
	.byte 2, 0x05
	.byte 3, 0x68
	.byte 4, 0x04
	.byte 3, 0x69
	.byte 4, 0x04
	.byte 3, 0x07
	.byte 4, 0x01
	.byte 3, 0x18
	.byte 4, 0x01
	.byte 3, 0x10
	.byte 2, 0x16
	.byte 2, 0x90
	.byte 3, 0x11
	.byte 2, 0x01
	.byte 2, 0x00
	.byte 3, 0x12
	.byte 2, 0x01
	.byte 2, 0x17
	.byte 3, 0x13
	.byte 2, 0x0f
	.byte 2, 0x80
	.byte 3, 0x12
	.byte 2, 0x01
	.byte 2, 0x37
	.byte 3, 0x20
	.byte 4, 0x00
	.byte 3, 0x21
	.byte 4, 0x00
	.byte 3, 0x50
	.byte 4, 0x00
	.byte 3, 0x51
	.byte 4, 0xaf
	.byte 3, 0x52
	.byte 4, 0x00
	.byte 3, 0x53
	.byte 4, 0x83
	.byte 3, 0x90
	.byte 4, 0x03
	.byte 3, 0x91
	.byte 4, 0x00
	.byte 3, 0x92
	.byte 2, 0x01
	.byte 2, 0x01
	.byte 3, 0x98
	.byte 2, 0x04
	.byte 2, 0x00
	.byte 3, 0x99
	.byte 2, 0x13
	.byte 2, 0x02
	.byte 3, 0x9a
	.byte 2, 0x02
	.byte 2, 0x02
	.byte 3, 0x9b
	.byte 2, 0x02
	.byte 2, 0x00
	.byte 5, 100
	.byte 3, 0x07
	.byte 4, 0x21
	.byte 3, 0x12
	.byte 2, 0x01
	.byte 2, 0x37
	.byte 5, 1
	.byte 3, 0x07
	.byte 4, 0x21
	.byte 3, 0x12
	.byte 2, 0x11
	.byte 2, 0x37
	.byte 5, 100
	.byte 3, 0x07
	.byte 2, 0x02
	.byte 2, 0x33
	.byte 0

lcd_inittab_type3:
	.byte 1, 0x01
	.byte 5, 5
	.byte 1, 0x11
	.byte 5, 120
	.byte 1, 0x3a
	.byte 2, 0x65
	.byte 1, 0xab
	.byte 1, 0x35
	.byte 2, 0x00
	.byte 1, 0xf2
	.byte 2, 0x01
	.byte 1, 0xe0
	.byte 2, 0x71
	.byte 2, 0x76
	.byte 2, 0x25
	.byte 2, 0x01
	.byte 2, 0xa5
	.byte 2, 0x09
	.byte 2, 0x15
	.byte 2, 0x11
	.byte 1, 0xe1
	.byte 2, 0x40
	.byte 2, 0x21
	.byte 2, 0x64
	.byte 2, 0x13
	.byte 2, 0xf3
	.byte 2, 0x0b
	.byte 2, 0x00
	.byte 2, 0x00
	.byte 1, 0xe2
	.byte 2, 0x71
	.byte 2, 0x65
	.byte 2, 0x24
	.byte 2, 0x08
	.byte 2, 0x97
	.byte 2, 0x01
	.byte 2, 0x15
	.byte 2, 0x11
	.byte 1, 0xe3
	.byte 2, 0x51
	.byte 2, 0x01
	.byte 2, 0x62
	.byte 2, 0x13
	.byte 2, 0xf3
	.byte 2, 0x0b
	.byte 2, 0x00
	.byte 2, 0x00
	.byte 1, 0xe4
	.byte 2, 0x71
	.byte 2, 0x57
	.byte 2, 0x31
	.byte 2, 0x01
	.byte 2, 0x82
	.byte 2, 0x04
	.byte 2, 0x1f
	.byte 2, 0x11
	.byte 1, 0xe5
	.byte 2, 0x64
	.byte 2, 0x41
	.byte 2, 0x64
	.byte 2, 0x19
	.byte 2, 0xb3
	.byte 2, 0x09
	.byte 2, 0x00
	.byte 2, 0x00
	.byte 1, 0x29
	.byte 0

lcd_inittab_type7:
	.byte 1, 0x01
	.byte 5, 10
	.byte 1, 0xb4
	.byte 2, 0x00
	.byte 1, 0xb6
	.byte 2, 0x01
	.byte 1, 0xb7
	.byte 2, 0x00
	.byte 2, 0x00
	.byte 2, 0x02
	.byte 2, 0x00
	.byte 2, 0x06
	.byte 2, 0x26
	.byte 2, 0x2d
	.byte 2, 0x27
	.byte 2, 0x55
	.byte 2, 0x27
	.byte 1, 0xb8
	.byte 2, 0x10
	.byte 1, 0xb9
	.byte 2, 0x52
	.byte 2, 0x12
	.byte 2, 0x03
	.byte 1, 0xc0
	.byte 2, 0x0a
	.byte 2, 0x10
	.byte 2, 0x10
	.byte 1, 0xc2
	.byte 2, 0x14
	.byte 2, 0x23
	.byte 1, 0xc3
	.byte 2, 0x12
	.byte 2, 0x23
	.byte 1, 0xc6
	.byte 2, 0x48
	.byte 1, 0xe0
	.byte 2, 0x20
	.byte 2, 0x71
	.byte 2, 0x17
	.byte 2, 0x09
	.byte 2, 0x70
	.byte 2, 0x0c
	.byte 2, 0x13
	.byte 2, 0x25
	.byte 1, 0xe1
	.byte 2, 0x37
	.byte 2, 0x00
	.byte 2, 0x63
	.byte 2, 0x11
	.byte 2, 0xd9
	.byte 2, 0x00
	.byte 2, 0x12
	.byte 2, 0x01
	.byte 1, 0xe2
	.byte 2, 0x42
	.byte 2, 0x42
	.byte 2, 0x60
	.byte 2, 0x08
	.byte 2, 0xb4
	.byte 2, 0x07
	.byte 2, 0x0e
	.byte 2, 0x90
	.byte 1, 0xe3
	.byte 2, 0x47
	.byte 2, 0x60
	.byte 2, 0x66
	.byte 2, 0x09
	.byte 2, 0x6a
	.byte 2, 0x02
	.byte 2, 0x0e
	.byte 2, 0x09
	.byte 1, 0xe4
	.byte 2, 0x11
	.byte 2, 0x40
	.byte 2, 0x03
	.byte 2, 0x0a
	.byte 2, 0xc1
	.byte 2, 0x0d
	.byte 2, 0x17
	.byte 2, 0x30
	.byte 1, 0xe5
	.byte 2, 0x00
	.byte 2, 0x30
	.byte 2, 0x77
	.byte 2, 0x1c
	.byte 2, 0xfb
	.byte 2, 0x00
	.byte 2, 0x13
	.byte 2, 0x07
	.byte 1, 0xe6
	.byte 2, 0x01
	.byte 1, 0x35
	.byte 2, 0x00
	.byte 1, 0x36
	.byte 2, 0x00
	.byte 1, 0xf2
	.byte 2, 0x40
	.byte 1, 0xf3
	.byte 2, 0x50
	.byte 1, 0xfb
	.byte 2, 0x01
	.byte 1, 0x11
	.byte 5, 200
	.byte 1, 0xb1
	.byte 2, 0x16
	.byte 2, 0x03
	.byte 1, 0xb2
	.byte 2, 0x17
	.byte 2, 0x03
	.byte 1, 0x3a
	.byte 2, 0x65
	.byte 1, 0x29
	.byte 0

.align 2

lcd_initmapping:
	.word	lcd_inittab_type3 - lcd_initmapping_ref
	.word	lcd_inittab_type7 - lcd_initmapping_ref
	.word	lcd_inittab_type2 - lcd_initmapping_ref
	.word	lcd_inittab_type3 - lcd_initmapping_ref

initlcd:
	stmfd	sp!, {r4,r5,lr}
	bl	detectlcd
	sub	r12, r12, #0x04900000
	adr	r5, lcd_initmapping
	add	r5, r5, r0,lsl#2
	ldr	r5, [r5]
	add	r5, r5, pc
	mov	r0, #0xc00
lcd_initmapping_ref:
	orr	r0, r0, #1
	str	r0, [r12]
	mov	r0, #0x7f00
	orr	r0, r0, #0xff
	str	r0, [r12,#0x24]
	mov	r0, #0
	str	r0, [r12,#0x28]
	mov	r0, #1
	bl	sleepms
	mov	r0, #1
	str	r0, [r12,#0x28]
	mov	r0, #5
	bl	sleepms
initlcd_loop:
	adr	lr, initlcd_loop
	ldrb	r1, [r5], #1
	ldrb	r0, [r5], #1
	add	pc, pc, r1,lsl#2
	nop
	ldmfd	sp!, {r4,r5,pc}
	b	sendlcdc
	b	sendlcdd
	b	sendlcd2c
	b	sendlcd2d
	b	sleepms

sendlcd2c:
	ldr	r4, [r12,#0x1c]
	ands	r4, r4, #0x10
	bne	sendlcd2c
	mov	r4, r0,lsr#8
	str	r4, [r12,#0x04]
	and	r0, r0, #0xff
sendlcdc:
	ldr	r4, [r12,#0x1c]
	ands	r4, r4, #0x10
	bne	sendlcdc
	str	r0, [r12,#0x04]
	mov	pc, lr
sendlcd2d:
	ldr	r4, [r12,#0x1c]
	ands	r4, r4, #0x10
	bne	sendlcd2d
	mov	r4, r0,lsr#8
	str	r4, [r12,#0x40]
	and	r0, r0, #0xff
sendlcdd:
	ldr	r4, [r12,#0x1c]
	ands	r4, r4, #0x10
	bne	sendlcdd
	str	r0, [r12,#0x40]
	mov	pc, lr

detectlcd:
	mov	r12, #0x3c000000
	orr	r12, r12, #0xf00000
	ldr	r0, [r12,#0xd0]
	bic	r0, r0, #0x0f
	str	r0, [r12,#0xd0]
	ldr	r0, [r12,#0xe0]
	bic	r0, r0, #0xf0
	str	r0, [r12,#0xe0]
	ldr	r0, [r12,#0xd4]
	and	r0, r0, #1
	ldr	r1, [r12,#0xe4]
	and	r1, r1, #2
	orr	r0, r0, r1
	mov	pc, lr

getpowerok:
	stmfd	sp!, {lr}
	mov	r0, #0xe6
	mov	r1, #0x19
	bl	i2crecvbyte
	eor	r0, r0, #1
	ands	r0, r0, #1
	ldmnefd	sp!, {pc}
	mov	r0, #0xe6
	mov	r1, #0x4b
	bl	i2crecvbyte
	ands	r0, r0, #4
	movne	r0, #1
	ldmnefd	sp!, {pc}
	mov	r0, #0xe6
	mov	r1, #0x12
	bl	i2crecvbyte
	ands	r0, r0, #4
	movne	r0, #1
	ldmfd	sp!, {pc}

poweroff:
	mov	r0, #0xe6
	mov	r1, #0x0c
	mov	r2, #0x01
	bl	i2csendbyte
	b	poweroff

i2csendbyte:
	mov	r3, #0
@fallthrough

i2csend:
	stmfd	sp!, {r4,lr}
	mov	r12, #0x3C000000
	add	r12, r12, #0x00900000
	mov	r4, #0
	str	r4, [r12,#0x08]
	str	r0, [r12,#0x0c]
	mov	r4, #0xf0
	str	r4, [r12,#0x04]
	mov	r4, #0xb7
	str	r4, [r12]
	bl	i2cwait
	str	r1, [r12,#0x0c]
	str	r4, [r12]
	bl	i2cwait
	movs	r3, r3
	moveq	r0, r2
i2csend_write:
	ldrne	r0, [r2], #1
	str	r0, [r12,#0x0c]
	str	r4, [r12]
	bl	i2cwait
	subs	r3, r3, #1
	bhi	i2csend_write
	mov	r0, #0xd0
	str	r0, [r12,#0x04]
	str	r4, [r12]
i2csend_wait:
	ldr	r0, [r12,#0x04]
	tst	r0, #0x20
	bne	i2csend_wait
	ldmfd	sp!, {r4,pc}

i2crecvbyte:
	mov	r2, #0
	mov	r3, #1
@fallthrough

i2crecv:
	stmfd	sp!, {r0,r4,lr}
	mov	r12, #0x3C000000
	add	r12, r12, #0x00900000
	mov	r4, #0
	str	r4, [r12,#0x08]
	str	r0, [r12,#0x0c]
	mov	r4, #0xf0
	str	r4, [r12,#0x04]
	mov	r4, #0xb7
	str	r4, [r12]
	bl	i2cwait
	str	r1, [r12,#0x0c]
	str	r4, [r12]
	bl	i2cwait
	ldr	r0, [sp]
	orr	r0, r0, #1
	str	r1, [r12,#0x0c]
	mov	r0, #0xb0
	str	r0, [r12,#0x04]
	str	r4, [r12]
	bl	i2cwait
i2crecv_read:
	subs	r3, r3, #1
	moveq	r4, #0x37
	str	r4, [r12]
	bl	i2cwait
	ldr	r0, [r12,#0x0c]
	movs	r2, r2
	strne	r0, [r2], #1
	movs	r3, r3
	bne	i2crecv_read
	mov	r1, #0x90
	str	r1, [r12,#0x04]
	mov	r1, #0xb7
	str	r1, [r12]
i2crecv_wait:
	ldr	r1, [r12,#0x04]
	tst	r1, #0x20
	bne	i2crecv_wait
	ldmfd	sp!, {r0,r4,pc}

i2cwait:
	ldr	r0, [r12]
	tst	r0, #0x10
	beq	i2cwait
	mov	pc, lr


sleep10ms:
	mov	r0, #10
@ fallthrough

sleepms:
	mov	r2, #0xc8
	mul	r1, r0, r2
	mov	r2, #0x3c000000
	orr	r2, r2, #0x700000
	ldr	r0, [r2,#0x80]
	ldr	r0, [r2,#0x84]
	add	r1, r1, r0
sleepmsloop:
	ldr	r0, [r2,#0x80]
	ldr	r0, [r2,#0x84]
	cmp	r0, r1
	bmi	sleepmsloop
	mov	pc, lr

_stubend:
