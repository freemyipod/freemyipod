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


#include "build/version.h"


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
	mov	r11, #0x22000000
	orr	r11, r11, #0x10000
	mov	r0, r11
	mov	r3, #0x2d00
	orr	r3, r3, #0x60
	sub	r2, r1, #1
fillbuff:
	str	r2, [r0], #4
	subs	r3, r3, #1
	bne	fillbuff
	mov	r0, r11
	adr	r3, text
	bl	rendertext
	add	r0, r11, #0xb00
	bl	rendertext
	add	r0, r11, #0x2100
	bl	rendertext
	mov	r0, #0
	mov	r1, #175
	mov	r2, #0
	mov	r3, #131
	str	r11, [sp]
	bl	displaylcd
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
	orr	r12, r12, #0xcf00000
	ldr	r0, [r12,#0xe4]
	tst	r0, #0x40
	beq	recoverymode
	b	decompress


rendertext:
	stmfd	sp!, {r4-r6,lr}
rendertext_next:
	ldrb	r12, [r3], #1
	cmp	r12, #0
	beq	rendertext_done
	cmn	r2, #1
	beq	rendertext_nobg
	mov	r6, r0
	mov	r4, #8
rendertext_bgrow:
	mov	r5, #6
rendertext_bgcol:
	cmn	r2, #2
	strneh	r2, [r6], #2
	bne	rendertext_noblend
	ldrh	lr, [r6]
	tst	lr, #1
	orrne	lr, lr, #0x10000
	mov	lr, lr,lsr#1
	bic	lr, lr, #0x1000
	bic	lr, lr, #0x84
	strh	lr, [r6], #2
rendertext_noblend:
	subs	r5, r5, #1
	bne	rendertext_bgcol
	add	r6, r6, #340
	subs	r4, r4, #1
	bne	rendertext_bgrow
rendertext_nobg:
	adr	r5, font
	sub	r12, r12, #0x20
	cmp	r12, #0x5f
	addcc	r5, r12,lsl#2
	addcc	r5, r12
	mov	r12, #5
rendertext_col:
	mov	r6, r0
	ldrb	r4, [r5], #1
rendertext_row:
	tst	r4, #1
	strneh	r1, [r6]
	add	r6, r6, #352
	movs	r4, r4,lsr#1
	bne	rendertext_row
	add	r0, r0, #2
	subs	r12, r12, #1
	bne	rendertext_col
	add	r0, r0, #2
	b	rendertext_next
rendertext_done:
	ldmfd sp!, {r4-r6,pc}


text:
	.ascii "emCORE Loader v"
	.ascii VERSION
	.ascii " r"
	.ascii VERSION_SVN
	.ascii "\0"
	.ascii "Switch HOLD on for recovery\0"
	.ascii "Loading emCORE...\0"


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


recoverytext:
	.ascii "Entered recovery mode\0"
	.ascii "Connect via USB\0"


.align 1


.code 16
thumb_nrv2e_d8:
	mov r7,r2
        mov r4,#1; neg r5,r4 @ r5= -1 initial condition
        lsl r4,#31 @ 1<<31: refill next time
        mov r6,#5
        lsl r6,#8 @ 0x500 @ nrv2e M2_MAX_OFFSET
        b top_n2e

nrv2e_done:
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
.align 2


decompress:
	mov	r0, #0x24000000
	add	r0, r0, #0xa000
	mov	r2, #0x08000000
	adr	r12, thumb_nrv2e_d8 + 1
	bx	r12


recoverymode:
	add	r0, r11, #0x2100
	mov	r1, #0
	mov	r2, #0xff00
	orr	r2, r2, #0xff
	adr	r3, recoverytext
	bl	rendertext
	add	r0, r11, #0x2c00
	bl	rendertext
	mov	r0, #0
	mov	r1, #175
	mov	r2, #0
	mov	r3, #131
	str	r11, [sp]
	bl	displaylcd
	mov	r12, #0
	mov	r11, #1
MOV R8, #0x38800000
MOV R9, #0x3C400000
ADD R1, R9, #0x00100000    @ Enable USB clocks
LDR R0, [R1,#0x28]
BIC R0, R0, #0x4000
STR R0, [R1,#0x28]
LDR R0, [R1,#0x40]
BIC R0, R0, #0x800
STR R0, [R1,#0x40]

LDR R0, [R8,#0xE00]        @ PHY clock enable
BIC R0, R0, #3
STR R0, [R8,#0xE00]

LDR SP, [R8,#0x804]
ORR R0, SP, #2
STR R0, [R8,#0x804]        @ USB2 Gadget: Soft disconnect

BL sleep10ms
MOV R9, #0x3C400000
STR R12, [R9]              @ USB2 PHY: Power on
STR R11, [R9,#0x1C]        @ USB2 PHY: Undocumented
MOV R0, #0xE00
ORR R0, R0, #0x3F
STR R0, [R9,#0x44]         @ USB2 PHY: Undocumented
LDR R0, [R9,#0x04]
BIC R0, R0, #3
STR R0, [R9,#0x04]         @ USB2 PHY: Clock is 48MHz
LDR R4, [R9,#0x08]
ORR R1, R4, #1
STR R1, [R9,#0x08]         @ USB2 PHY: Assert Software Reset
BL sleep10ms
STR R12, [R9,#0x08]         @ USB2 PHY: Deassert Software Reset
BL sleep10ms

STR R11, [R8,#0x10]        @ USB2 Gadget: Assert Core Software Reset
waitcorereset:
LDR R0, [R8,#0x10]         @ USB2 Gadget: Wait for Core to reset
TST R0, #1
BNE waitcorereset
TST R0, #0x80000000        @ USB2 Gadget: Wait for AHB IDLE
BEQ waitcorereset

MOV R0, #0xB6
STR R0, [R8,#0x24]         @ USB2 Gadget: RX FIFO size: 728 bytes
ORR R0, R0, #0x840000
STR R0, [R8,#0x28]         @ USB2 Gadget: Non-periodic TX FIFO size: 528 bytes
MOV R0, #0x26
STR R0, [R8,#0x08]         @ USB2 Gadget: DMA Enable, Burst Length: 4, Mask Interrupts
MOV R0, #0x1400
ADD R0, R0, #8
STR R0, [R8,#0x0C]         @ USB2 Gadget: PHY IF is 16bit, Turnaround 5
STR SP, [R8,#0x804]        @ USB2 Gadget: Soft reconnect

@ fallthrough

mainloop:
  LDR R3, [R8,#0x14]         @ Global USB interrupts
  TST R3, #0x00001000        @ BUS reset
  BEQ noreset
    MOV R2, #0x500
    STR R2, [R8,#0x804]
    MOV R2, #4
    STR R2, [R8,#0x800]        @ USB2 Gadget: Device Address 0, STALL on non-zero length status stage
    MOV R2, #0x8000
    STR R2, [R8,#0x900]        @ USB2 Gadget: Endpoint 0 IN Control: ACTIVE
    STR R2, [R8,#0xB00]        @ USB2 Gadget: Endpoint 0 OUT Control: ACTIVE
    SUB R5, R12, #1
    STR R5, [R8,#0x908]        @ USB2 Gadget: Endpoint 0 IN Interrupt: ALL
    STR R5, [R8,#0xB08]        @ USB2 Gadget: Endpoint 0 OUT Interrupt: ALL
    LDR R2, val_20080040
    STR R2, [R8,#0xB10]        @ USB2 Gadget: Endpoint 0 OUT Transfer Size: 64 Bytes, 1 Packet, 1 Setup Packet
    MOV R2, #0x22000000
    ORR R2, R2, #0x10000
    STR R2, [R8,#0xB14]        @ USB2 Gadget: Endpoint 0 OUT DMA Address: 0x22010000
    LDR R2, [R8,#0xB00]
    ORR R2, R2, #0x84000000
    STR R2, [R8,#0xB00]        @ USB2 Gadget: Endpoint 0 OUT Control: ENABLE CLEARNAK
    LDR R2, val_00088210
    STR R2, [R8,#0x960]        @ USB2 Gadget: Endpoint 3 IN Control: ACTIVE BULK, 528 byte packets
    STR R2, [R8,#0xB80]        @ USB2 Gadget: Endpoint 4 OUT Control: ACTIVE BULK, 528 byte packets
    STR R5, [R8,#0x968]        @ USB2 Gadget: Endpoint 3 IN Interrupt: ALL
    STR R5, [R8,#0xB88]        @ USB2 Gadget: Endpoint 4 OUT Interrupt: ALL
    LDR R2, val_00080210
    STR R2, [R8,#0xB90]        @ USB2 Gadget: Endpoint 4 OUT Transfer Size: 528 Bytes, 1 Packet
    MOV R2, #0x22000000
    ORR R2, R2, #0x12000
    STR R2, [R8,#0xB94]        @ USB2 Gadget: Endpoint 4 OUT DMA Address: 0x22012000
    LDR R2, [R8,#0xB80]
    ORR R2, R2, #0x94000000
    STR R2, [R8,#0xB80]        @ USB2 Gadget: Endpoint 4 OUT Control: ENABLE CLEARNAK DATA0
    STR R5, [R8,#0x810]        @ USB2 Gadget: IN Endpoint Interrupt Mask: ALL
    STR R5, [R8,#0x814]        @ USB2 Gadget: OUT Endpoint Interrupt Mask: ALL
    STR R5, [R8,#0x81C]        @ USB2 Gadget: Enable interrupts on all endpoints
  noreset:
  TST R3, #0x00040000        @ IN endpoint event
  BEQ noinevent
    LDR R4, [R8,#0x908]        @ Just ACK them all...
    STR R4, [R8,#0x908]
    LDR R4, [R8,#0x968]
    STR R4, [R8,#0x968]
  noinevent:
  TST R3, #0x00080000        @ OUT endpoint event
  BEQ nooutevent
    LDR R4, [R8,#0xB08]
    MOVS R4, R4                @ Event on OUT EP0
    BEQ noep0out
      TST R4, #8                 @ SETUP phase done
      BEQ controldone
        BL flushcache
        MOV R5, #0x22000000
        ORR R5, R5, #0x10000
        LDRB R6, [R5,#0x01]        @ Get request type
        CMP R6, #0
          BEQ GET_STATUS
        CMP R6, #1
          BEQ CLEAR_FEATURE
        CMP R6, #3
          BEQ SET_FEATURE
        CMP R6, #5
          BEQ SET_ADDRESS
        CMP R6, #6
          BEQ GET_DESCRIPTOR
        CMP R6, #8
          BEQ GET_CONFIGURATION
        CMP R6, #9
          BEQ SET_CONFIGURATION
        ctrlstall:
        LDR R1, [R8,#0x900]
        ORR R1, R1, #0x00200000
        STR R1, [R8,#0x900]        @ Stall IN EP0
        LDR R1, [R8,#0xB00]
        ORR R1, R1, #0x00200000
        STR R1, [R8,#0xB00]        @ Stall OUT EP0
      controldone:
      LDR R1, val_20080040
      STR R1, [R8,#0xB10]        @ OUT EP0: 64 Bytes, 1 Packet, 1 Setup Packet
      MOV R1, #0x22000000
      ORR R1, R1, #0x10000
      STR R1, [R8,#0xB14]        @ OUT EP0: DMA address
      LDR R1, [R8,#0xB00]
      ORR R1, R1, #0x84000000
      STR R1, [R8,#0xB00]        @ OUT EP0: Enable ClearNAK
    noep0out:
    STR R4, [R8,#0xB08]        @ ACK it, whatever it was...
    LDR R4, [R8,#0xB88]
    MOVS R4, R4                @ Event on OUT EP4
    BEQ noep1out
      TST R4, #1                 @ XFER complete
      BEQ datadone
        BL flushcache
        MOV R0, #0x22000000
        ORR LR, R0, #0x800
        ORR R0, R0, #0x12000
        LDR R1, [R0]
        LDR R2, [R0,#0x04]
        CMP R1, #0                 @ EXECUTE, no feedback
          MCREQ p15, 0, R12,c7,c5    @ Flush ICache
          LDREQ SP, [R0,#0x08]
          MOVEQ PC, R2
        CMP R1, #1                 @ READ
        BNE noread
          LDR R1, [R0,#0x08]
          MOV R6, R1,LSL#2
          MOV R0, #0x22000000
          ORR R0, R0, #0x13000
          ORR R0, R0, #0x10
          copydata:
            LDR R5, [R2], #4
            STR R5, [R0], #4
            SUBS R1, R1, #1
          BNE copydata
          ADD R1, R6, #0x10
          B sendsuccesscustomsize
        noread:
        CMP R1, #2                 @ WRITE
        BNE sendunknownfunc
          LDR R1, [R0,#0x08]
          ADD R0, R0, #0x10
          copydata2:
            LDR R5, [R0], #4
            STR R5, [R2], #4
            SUBS R1, R1, #1
          BNE copydata2
	  BL flushcache
          MOV R1, #0x10
          @ fallthrough
        sendsuccesscustomsize:
          MOV R2, #0x22000000
          ORR R2, R2, #0x13000
          STMIA R2, {R11, R12}
          @ fallthrough
        sendlast2zero:
          STR R12, [R2,#0x08]
          STR R12, [R2,#0x0C]
          @ fallthrough
        datasend:
          BL flushcache
          LDR R0, val_00088210
          STR R0, [R8,#0x960]        @ EP3 IN: ACTIVE BULK, 528 byte packets
          ORR R1, R1, #0x20000000    @ 1 Packet at a time
          ORR R1, R1, #0x00080000    @ 1 Packet
          STR R1, [R8,#0x970]        @ EP3 IN: 1 Packet, 1 Packet at a time, Size as in R1
          STR R2, [R8,#0x974]        @ EP3 IN: DMA address
          LDR R1, [R8,#0x960]
          ORR R1, R1, #0x84000000
          STR R1, [R8,#0x960]        @ EP3 IN: Enable ClearNAK
      datadone:
      LDR R1, val_00080210
      STR R1, [R8,#0xB90]        @ OUT EP4: 528 Bytes, 1 Packet
      MOV R1, #0x22000000
      ORR R1, R1, #0x12000
      STR R1, [R8,#0xB94]        @ Out EP4: DMA address
      LDR R1, [R8,#0xB80]
      ORR R1, R1, #0x84000000
      STR R1, [R8,#0xB80]        @ Out EP4: Enable ClearNAK
    noep1out:
    STR R4, [R8,#0xB88]        @ ACK it, whatever it was...
  nooutevent:
  STR R3, [R8,#0x14]         @ ACK it, whatever it was...
B mainloop

sendunknownfunc:
  MOV R0, #2
  MOV R1, #0x10
  MOV R2, #0x08400000
  STMIA R2, {R0, R12}
B sendlast2zero

GET_DESCRIPTOR:
  LDRB R7, [R5,#3]           @ Descriptor type
  CMP R7, #1
    ADREQ R0, devicedescriptor
    BEQ senddescriptor
  CMP R7, #2
    ADREQ R0, configurationdescriptor
    MOVEQ R1, #0x20
    BEQ senddescriptorcustomsize
  CMP R7, #3
  BNE ctrlstall
  LDRB R7, [R5,#2]           @ String descriptor index
  CMP R7, #0
    ADREQ R0, langstringdescriptor
    BEQ senddescriptor
  CMP R7, #1
  CMPNE R7, #2
    ADREQ R0, devnamestringdescriptor
  BNE ctrlstall
@ fallthrough

senddescriptor:
  LDRB R1, [R0]              @ Descriptor length
@ fallthrough

senddescriptorcustomsize:
  LDRH R5, [R5,#0x06]        @ Requested length
  CMP R5, R1
  MOVLO R1, R5
  MOV R2, #0x22000000
  ORR R2, R2, #0x11000
  ADD R6, R1, R2
  copydescriptor:
    LDR R5, [R0], #4
    STR R5, [R2], #4
    CMP R2, R6
  BCC copydescriptor
B ctrlsend

GET_STATUS:
  LDRB R1, [R5]
  CMP R1, #0x80
  MOV R0, #0x22000000
  ORR R0, R0, #0x11000
  STREQ R11, [R0]
  STRNE R12, [R0]
  MOV R1, #0x00000002
B ctrlsend

CLEAR_FEATURE:
  LDRB R2, [R5]
  CMP R2, #2
  LDREQ R2, [R5,#2]
  BICEQ R2, R2, #0x00800000
  CMPEQ R2, #0x00010000
@ fallthrough

SET_CONFIGURATION:
  LDREQ R2, [R8,#0x960]
  ORREQ R2, R2, #0x10000000
  STREQ R2, [R8,#0x960]      @ EP3 IN: Set DATA0 PID
  LDREQ R2, [R8,#0xB80]
  ORREQ R2, R2, #0x10000000
  STREQ R2, [R8,#0xB80]      @ EP4 OUT: Set DATA0 PID
B SET_FEATURE              @ zero-length ACK

SET_ADDRESS:
  LDRH R2, [R5,#0x02]        @ new address
  LDR R1, [R8,#0x800]
  BIC R1, R1, #0x000007F0
  ORR R1, R1, R2,LSL#4
  STR R1, [R8,#0x800]        @ set new address
@ fallthrough

SET_FEATURE:
  MOV R1, #0                 @ zero-length ACK
B ctrlsend

GET_CONFIGURATION:
  MOV R1, #0x00000001
  STR R1, [R0]
@ fallthrough

ctrlsend:
  BL flushcache
  MOV R0, #0x22000000        @ Buffer to be sent
  ORR R0, R0, #0x11000
  MOV R2, #0x00009800
  STR R2, [R8,#0x900]        @ EP0 IN: ACTIVE
  ORR R1, R1, #0x00080000    @ 1 Packet
  STR R1, [R8,#0x910]        @ EP0 IN: 1 Packet, Size as in R1
  STR R0, [R8,#0x914]        @ EP0 IN: DMA address
  LDR R1, [R8,#0x900]
  ORR R1, R1, #0x84000000
  STR R1, [R8,#0x900]        @ EP0 IN: Enable ClearNAK
B controldone

val_00080210:
.word 0x00080210

val_00088210:
.word 0x00088210

val_20080040:
.word 0x20080040

devicedescriptor:
.word 0x02000112
.word 0x40FFFFFF
.word 0xE112FFFF
.word 0x02010001
.word 0x00010100

configurationdescriptor:
.word 0x00200209
.word 0xC0000101
.word 0x00040932
.word 0xFFFF0200
.word 0x050700FF
.word 0x02100204
.word 0x83050701
.word 0x01021002

langstringdescriptor:
.word 0x04090304

devnamestringdescriptor:
.byte 0x38
.byte 3
.ascii "e\0m\0C\0O\0R\0E\0 \0L\0o\0a\0d\0e\0r\0 \0R\0e\0c\0o\0v\0e\0r\0y\0 \0I\0N\0\062\0G\0"


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

displaylcd:
	stmfd	sp!, {r0,r1,r4,lr}
	bl	displaylcd_sync
	bl	detectlcd
	sub	r12, r12, #0x04900000
	cmp	r0, #2
	bne	displaylcd_othertypes
	mov	r0, #0x50
	bl	sendlcd2c
	ldr	r0, [sp]
	bl	sendlcd2d
	mov	r0, #0x51
	bl	sendlcd2c
	ldr	r0, [sp,#0x04]
	bl	sendlcd2d
	mov	r0, #0x52
	bl	sendlcd2c
	mov	r0, r2
	bl	sendlcd2d
	mov	r0, #0x53
	bl	sendlcd2c
	mov	r0, r3
	bl	sendlcd2d
	mov	r0, #0x20
	bl	sendlcd2c
	ldr	r0, [sp]
	bl	sendlcd2d
	mov	r0, #0x21
	bl	sendlcd2c
	mov	r0, r2
	bl	sendlcd2d
	mov	r0, #0x22
	bl	sendlcd2c
	b	displaylcd_blit
displaylcd_othertypes:
	mov	r0, #0x2a
	bl	sendlcdc
	ldr	r0, [sp]
	bl	sendlcd2d
	ldr	r0, [sp,#0x04]
	bl	sendlcd2d
	mov	r0, #0x2b
	bl	sendlcdc
	mov	r0, r2
	bl	sendlcd2d
	mov	r0, r3
	bl	sendlcd2d
	mov	r0, #0x2c
	bl	sendlcdc
displaylcd_blit:
	ldmia	sp, {r0,r1}
	sub	r1, r0
	add	r1, r1, #1
	sub	r3, r2
	add	r3, r3, #1
	mul	r2, r1, r3
	ldr	r1, [sp,#0x10]
	cmp	r1, #0x40000000
	bne	displaylcd_dma
displaylcd_pixel:
	ldr	r0, [sp,#0x14]
	bl	sendlcd2d
	subs	r2, r2, #1
	bne	displaylcd_pixel
	ldmfd	sp!, {r0,r1,r4,pc}
displaylcd_dma:
	mov	r3, #0x38000000
	add	r3, r3, #0x400000
	mov	r0, #0x20000000
	orr	r0, r0, #0x180000
	str	r0, [r3,#0x104]
	str	r1, [r3,#0x100]
	mov	r0, r2,lsr#1
	sub	r0, r0, #1
	str	r0, [r3,#0x108]
	bl	flushcache
	mov	r0, #4
	str	r0, [r3,#0x114]
	ldmfd	sp!, {r0,r1,r4,pc}

displaylcd_sync:
	mov	r1, #0x38000000
	add	r1, r1, #0x400000
displaylcd_sync_wait:
	ldr	r0, [r1,#0x184]
	tst	r0, #0x40000
	bne	displaylcd_sync_wait
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
