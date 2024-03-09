@
@
@    emCORE Loader for iPod Nano 4G
@
@    Copyright 2011 TheSeven, Farthen
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
	mcr	p15, 0, r0,c1,c0 @ disable mmu

	mov	lr, #0
	adr	sp, values1
	mcr	p15, 0, lr,c7,c14,0 @ clean & invalidate data cache
	mcr	p15, 0, lr,c7,c10,4 @ drain write buffer
	mcr	p15, 0, lr,c7,c5    @ invalidate instruction cache
	mcr	p15, 0, lr,c7,c5,4  @ flush prefetch buffer
	mcr	p15, 0, lr,c8,c7    @ invalidate all unlocked entries in the TLB
	mcr	p15, 0, lr,c13,c0,0 @ disable context id register
    

.macro block0_constpool    @ Block 0 (MMU, DMA) register map:
                           @ R0: Unused
                           @ R1: Unused
                           @ R2: Unused
                           @ R3: Scratchpad
	.word	0x00000FFF @ R4: DMA maximum transfer size
	.word	0x00012C00 @ R5: LCD framebuffer pixel count
	.word	0x22006800 @ R6: LCD framebuffer address
	.word	0x38300040 @ R7: LCD data port address
	.word	0x2202D000 @ R8: LCD DMA linked list address
	.word	0x74240000 @ R9: LCD DMA channel settings
	.word	0x0005187D @ R10: CP15r1
	.word	0x2202C000 @ R11: First level page table
	.word	0x00000C1E @ R12: Default segment flags
                           @ R13: Constant pool pointer
.endm                      @ R14: 0

	ldmia	sp!, {r4-r12}
	mcr	p15, 0, r11,c2,c0 @ set first level translation table
	mov	r3, #-1
	mcr	p15, 0, r3,c3,c0 @ disable domain access control                  @ R3: Unused
	orr	r0, r12, #0x22000000
	str	r0, [r11], #4
	add	r12, r12, #0x00100000
mmuloop:
	str	r12, [r11], #4
	add	r12, r12, #0x00100000
	cmp	r12, #0x38000000
	biccs	r12, r12, #0xc
	tst	r12, #0x40000000
	beq	mmuloop
                                                                                  @ R11: LCD DMA descriptor head
                                                                                  @ R12: Unused
	mcr	p15, 0, r10,c1,c0                                                 @ R10: Unused

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

.macro block1_constpool    @ Block 1 (SYSCON) register map:
	.word	0x000327E5 @ R0: PWRCON(0)
	.word	0xFE2BED6D @ R1: PWRCON(1)
	.word	0x00DCF779 @ R2: PWRCON(4)
	.word	0x003E3E00 @ R3
	.word	0x06008501 @ R4
	.word	0x00009DBC @ R5
	.word	0x00404040 @ R6
	.word	0x40001000 @ R7
	.word	0x80008000 @ R8
	.word	0x38501000 @ R9
	.word	0xE02BED4D @ R10: PWRCON(1) during timer setup
	.word	0x001CF700 @ R11: PWRCON(4) during timer setup
	.word	0x3C500000 @ R12: SYSCON base address
                           @ R13: Constant pool pointer
.endm                      @ R14: 0

	ldmia	sp!, {r0-r12}
	str	r0, [r12,#0x48]  @ PWRCON0 ...                                    @ R0: Scratchpad
	str	r1, [r12,#0x4c]
	mov	r0, #0x73
	str	r0, [r12,#0x58]
	mov	r0, #0xff
	str	r0, [r12,#0x68]
	str	r2, [r12,#0x6c]  @ ... PWRCON4
	str	lr, [r12]
sysconwait1:
	ldr	r0, [r12]
	tst	r0, #0xf
	bne	sysconwait1      @ while ([SYSCON] & 0xf)
    
	str	lr, [r12,#0x04]
sysconwait2:
	ldr	r0, [r12,#0x04]
	tst	r0, r3
	bne	sysconwait2      @ while ([SYSCON+4] & 0x003E3E00)
    
	mov	r0, #0x7
	str	r0, [r12,#0x44]
	str	lr, [r12,#0x44]
	str	lr, [r12,#0x3c]
	str	r4, [r12,#0x20]                                                   @ R4: Scratchpad
	str	r5, [r12,#0x30]
	mov	r5, #1                                                            @ R5: 1
	str	r5, [r12,#0x44]
	orr	r4, r0, #0x10000
	str	r4, [r12,#0x44]
sysconwait3:
	ldr	r4, [r12,#0x40]
	tst	r4, #0x1
	beq	sysconwait3      @ while (!([SYSCON+0x40] & 1))
    
	str	r6, [r12,#0x04]                                                   @ R6: Unused
	add	r3, r3, #0x3e                                                     @ R3: 0x003E3E3E
sysconwait4:
	ldr	r4, [r12,#0x04]
	tst	r4, r3                                                            @ R3: Unused
	bne	sysconwait4
    
	str	r7, [r12]                                                         @ R7: Unused
sysconwait5:
	ldr	r2, [r12]
	tst	r4, #0xf
	bne	sysconwait5
    
	str	r8, [r12,#0x08]
	orr	r4, r8, r8,lsr#1                                                  @ R8: Unused
	str	r4, [r12,#0x0c]
	mov	r4, #0xc000
	str	r4, [r12,#0x10]
	mov	r4, #0x8000
	str	r4, [r12,#0x14]
	str	r4, [r12,#0x70]
	mov	r4, #2
	str	r4, [r9]                                                          @ R9: Unused
	mov	r0, #0x10                                                         @ R5: 0x10

                           @ Block 2 (TIMER) register map:
                           @ R0: 1
                           @ R1: PWRCON(1)
                           @ R2: PWRCON(4)
                           @ R3: Scratchpad
                           @ R4: Scratchpad
                           @ R5: 0x10
                           @ R6: Unused
                           @ R7: Unused
                           @ R8: Unused
                           @ R9: Unused
                           @ R10: PWRCON(1) during timer setup
                           @ R11: PWRCON(4) during timer setup
                           @ R12: SYSCON base address
                           @ R13: Constant pool pointer
                           @ R14: 0

	str	r10, [r12,#0x4c]     @ PWRCON(1) for timer setup                  @ R10: Unused
	mov	r4, #0x13
	str	r4, [r12,#0x58]             @ PWRCON(2) for timer setup
	str	r11, [r12,#0x6c]     @ PWRCON(4) for timer setup                  @ R11: Unused
	orr     r11, r12, #0x00200000                                             @ R11: TIMER base address
	str	r0, [r11,#0x4]       @ TACMD = 0x10
	str	r0, [r11,#0x24]      @ TBCMD = 0x10
	str	r0, [r11,#0x44]      @ TCCMD = 0x10
	str	r0, [r11,#0x64]      @ TDCMD = 0x10
	mov	r3, #0x40
	str	r3, [r11,#0xa0]      @ TECON = 0x40
	mov	r3, #0xb
	str	r3, [r11,#0xb0]      @ TEPRE = 0xb
	mov	r4, #-1                                                           @ R4: -1
	str	r4, [r11,#0xa8]      @ TEDATA0 = 0xFFFFFFFF
	mov	r3, #0x3
	str	r3, [r11,#0xa4]      @ TECMD = 0x3
	str	r0, [r11,#0xc4]      @ TFCMD = 0x10
	str	r0, [r11,#0xe4]      @ TGCMD = 0x10
	str	r0, [r11,#0x104]     @ THCMD = 0x10
	str	r4, [r11,#0x118]     @ THCMD = 0xFFFFFFFF
	str	r1, [r12,#0x4c]      @ PWRCON(1)                                  @ R1: Unused
	mov	r3, #0x73
	str	r3, [r12,#0x58]      @ PWRCON(2)
	str	r2, [r12,#0x6c]      @ PWRCON(4)                                  @ R2: Unused
	orr	r10, r11, #0x00800000                                             @ R10: GPIO base address

                           @ Block 3 (GPIO) register map:
                           @ R0: Unused
                           @ R1: Unused
                           @ R2: Unused
                           @ R3: Scratchpad
                           @ R4: -1
                           @ R5: 1
                           @ R6: Unused
                           @ R7: Unused
                           @ R8: Unused
                           @ R9: Unused
                           @ R10: GPIO base address
                           @ R11: TIMER base address
                           @ R12: SYSCON base address
                           @ R13: Constant pool pointer
                           @ R14: 0

.macro gpio_initdata
	.word	0x3202EEEE @ PCON0
	.word	0xE0EE2253 @ PCON1
	.word	0x2223EEEE @ PCON2
	.word	0x33333332 @ PCON3
	.word	0xFF333E33 @ PCON4
	.word	0xE0FEE200 @ PCON5
	.word	0x2222222E @ PCON6
	.word	0x22222222 @ PCON7
	.word	0xEEEEEEE2 @ PCON8
	.word	0xEEE0EEEE @ PCON9
	.word	0x2EEEEEEE @ PCONA
	.word	0xEEEE0222 @ PCONB
	.word	0xEEEEE00E @ PCONC
	.word	0xEEEEEEEE @ PCOND
	.word	0xEEEEEEEE @ PCONE
.endm

	ldr	r3, [sp], #0x4
	str	r3, [r10], #0xc                                                   @ R10: PCONx iterator
	mov	r3, #0x20
	str	r3, [r10], #0x4     @ PCON0 + 0xc = 0x20
	mov	r3, #0x40
	str	r3, [r10], #0x10    @ PCON0 + 0x10 = 0x40
	add	r9, r10, #0x1a0                                                   @ R9: Iterator limit
gpioloop1:
	ldr	r3, [sp], #0x4
	str	r3, [r10], #0xc
	str	lr, [r10], #0x4     @ PCON + 0xc = 0
	str	lr, [r10], #0x10    @ PCON + 0x10 = 0
	cmp	r10, r9
	bls	gpioloop1
                                                                                  @ R10: 0x3CF001E0
	ldr	r3, [r10,#0x1a8]
	bic	r3, r3, #2
	orr	r3, r3, #1
	str	r3, [r10,#0x1a8]
	sub	r8, r11, #0x00300000                                              @ R8: 0x39700000 (iterator)
	mov	r9, #6                                                            @ R9: Iterations remaining
gpioloop2:
	str	r14, [r8,#0x80]
	str	r4, [r8,#0xa0]
	str	r14, [r8,#0xc0]
	str	r14, [r8,#0xe0]
	add	r8, r8, #4
	subs	r9, r9, #1
	bne	gpioloop2
                                                                                  @ R9: 0
	ldr	r8, [r10,#-0x180]                                                 @ R8: PCON3 backup
	and	r3, r8, #0xff
	str	r3, [r10,#-0x180]   @ *PCON3 &= 0xff
	mov	r0, #0x3e8                                                        @ R0: Scratchpad
	bl	udelay                                                            @ R14: Return address
	ldr	r3, [r10,#-0x17c]
	and	r3, r3, #0xfc
	mov	r6, r3, lsr #0x2                                                  @ R6: Data for first PMU access
	str	r8, [r10,#-0x180]
	bic	r10, r11, #0x00100000                                             @ R10: I2C base address

                           @ Block 4 (I2C) register map:
                           @ R0: Scratchpad
                           @ R1: Scratchpad
                           @ R2: Unused
                           @ R3: Unused
                           @ R4: -1
                           @ R5: 1
                           @ R6: Data for first PMU access
                           @ R7: Unused
                           @ R8: Unused
                           @ R9: 0
                           @ R10: I2C base address
                           @ R11: TIMER base address
                           @ R12: SYSCON base address
                           @ R13: Constant pool pointer
                           @ R14: Return address / Scratchpad

	bl	i2cwaitrdy
	mov	r1, #0x40
	str	r1, [r10,#0x08]
	bl	i2cwaitrdy
	str	r5, [r10,#0x14]
	bl	i2cwaitrdy
	str	r9, [r10,#0x18]
	bl	i2cwaitrdy
	mov	r0, #0x80
	str	r0, [r10,#0x04]
	bl	i2cwaitrdy
	str	r9, [r10]
	bl	i2cwaitrdy
	str	r9, [r10,#0x04]
	bl	i2cwaitrdy
	str	r1, [r10,#0x0c]
	bl	i2cwaitrdy
	orr	r0, r5, #0x180
	str	r0, [r10]
	bl	i2cwaitrdy
	mov	r0, #0x10
	str	r0, [r10,#0x04]
	bl	i2cwaitrdy
    
                           @ Block 5 (PMU) register map:
                           @ R0: Address / Scratchpad (trashed by pmubatch)
                           @ R1: Data / Scratchpad (trashed by pmubatch)
                           @ R2: Scratchpad (set to 0xb7 by pmu accesses)
                           @ R3: Scratchpad (set to 0x10 by pmu accesses)
                           @ R4: Scratchpad (trashed by pmu accesses)
                           @ R5: 1
                           @ R6: Data for first PMU access
                           @ R7: Scratchpad (trashed by pmubatch)
                           @ R8: Used to store warmboot flag
                           @ R9: 0 / pmubatch transfer count (reset to 0 by pmubatch)
                           @ R10: I2C base address
                           @ R11: TIMER base address
                           @ R12: SYSCON base address
                           @ R13: Constant pool / pmubatch data pointer
                           @ R14: Return address / Scratchpad

	mov	r0, #0x7f
	mov	r1, r6
	bl	pmuwrite
	mov	r0, #0x02
	bl	pmuread
	ands	r8, r1, #0x80                                                     @ R8: Warmboot flag
	beq	pmu_coldboot
	mov	r1, #0x80
	bl	pmuwrite
pmu_coldboot:
.macro pmu_batch_1
pmu_batch_1_begin:
	.byte	0x14, 0x13
	.byte	0x15, 0x0d
	.byte	0x0b, 0x22
pmu_batch_1_end:
.endm
	mov	r9, #(pmu_batch_1_end - pmu_batch_1_begin) / 2
	bl	pmubatch
	tst	r6, #1                                                            @ R6: Unused
	beq	pmu_skip
	mov	r0, #0x0d
	bl	pmuread
	and	r1, r1, #0xdf
	bl	pmuwrite
pmu_skip:
.macro pmu_batch_2
pmu_batch_2_begin:
	.byte	0x1f, 0x14
	.byte	0x1a, 0xb2
	.byte	0x1a, 0xb2
	.byte	0x19, 0x14
	.byte	0x21, 0x06
	.byte	0x1d, 0x12
pmu_batch_2_end:
.endm
	mov	r9, #(pmu_batch_2_end - pmu_batch_2_begin) / 2
	bl	pmubatch
	mov	r0, #0x10
	bl	pmuread
	bic	r1, r1, #0x80
	orr	r1, r1, #0x60
	bl	pmuwrite
.macro pmu_batch_3
pmu_batch_3_begin:
	.byte	0x44, 0x72
pmu_batch_3_end:
.endm
	mov	r9, #(pmu_batch_3_end - pmu_batch_3_begin) / 2
	bl	pmubatch
	mov	r0, #0x40
	bl	pmuread
	orr	r1, r1, #0x40
	bl	pmuwrite
	mov	r0, #0x33
	bl	pmuread
	and	r1, r1, #0x03
	orr	r1, r1, #0x50
	bl	pmuwrite
	mov	r0, #0x34
	bl	pmuread
	and	r1, r1, #0x80
	orr	r1, r1, #0x54
	bl	pmuwrite
.macro pmu_batch_4
pmu_batch_4_begin:
	.byte	0x22, 0x00
	.byte	0x07, 0x50
	.byte	0x08, 0xfe
	.byte	0x09, 0x2b
	.byte	0x01, 0xff
	.byte	0x02, 0xff
	.byte	0x03, 0xff
pmu_batch_4_end:
.endm
	mov	r9, #(pmu_batch_4_end - pmu_batch_4_begin) / 2
	bl	pmubatch
	cmp	r8, #0
	bne	pmu_warmboot
	mov	r0, #0x30
	mov	r1, #0x64
	bl	pmuwrite
pmu_warmboot:
	mov	r0, #0x31
	bl	pmuread
	bic	r1, r1, #0x01
	bl	pmuwrite
.macro pmu_batch_5
pmu_batch_5_begin:
	.byte	0x0a, 0x70
	.byte	0x13, 0x02
pmu_batch_5_end:
.endm
	mov	r9, #(pmu_batch_5_end - pmu_batch_5_begin) / 2
	bl	pmubatch

	orr	lr, r11, #0x00800000                                              @ R14: GPIO base address
	str	r9, [lr,#0x384]
	orr	lr, r11, #0x01000000                                              @ R14: MIU base address
	str	r5, [lr]
	ldrh	r0, [sp], #2
	str	r0, [lr,#0x100]
	mov	r0, #0xff
	str	r0, [lr,#0x11c]
	str	r0, [lr,#0x120]
	
.macro block6_constpool           @ Block 6 (SDRAM) register map:
	.hword	0x1030
	.word	0x008AAC25 @ R0
	.word	0x050D67E5 @ R1
	.word	0x0002000B @ R2
	.word	0x0003B3B2 @ R3
	.word	0xFF53B3B0 @ R4
	.word	0x00008040 @ R5
	.word	0x8000100F @ R6: For LCD init at end of block
	.word	0x41000c20 @ R7: For LCD init at end of block
                           @ R8: Warmboot flag
                           @ R9: 0
                           @ R10: I2C base address
                           @ R11: TIMER base address
                           @ R12: SYSCON base address
                           @ R13: Constant pool pointer
.endm                      @ R14: MIU base address

	ldmia	sp!, {r0-r7}
	str	r0, [lr,#0x114]                                                   @ R0: Unused
	str	r1, [lr,#0x124]                                                   @ R1: Unused
	mov	r0, #8                                                            @ R0: Scratchpad
	str	r0, [lr,#0x118]
	str	r2, [lr,#0x108]
	mov	r0, #4
	str	r0, [lr,#0x148]
	str	r9, [lr,#0x14c]
	str	r3, [lr,#0x140]
miu_wait1:
	ldr	r0, [lr,#0x140]
	tst	r0, #2
	beq	miu_wait1
	add	r0, r3, #1                                                        @ R3: Unused
	str	r0, [lr,#0x140]
miu_wait2:
	ldr	r0, [lr,#0x144]
	mvn	r0, r0
	tst	r0, #3
	bne	miu_wait2
	ldr	r1, [lr,#0x144]                                                   @ R1: Scratchpad
	mov	r0, #0x0ff00000
	and	r0, r0, r1, lsl#2
	add	r0, r0, r4                                                        @ R4: Unused
	str	r0, [lr,#0x140]
	mov	r0, #0x10
	str	r0, [lr,#0x150]
	cmp	r8, #0                                                            @ R8: Unused
	sub	r8, r10, #0x04300000                                              @ R8: LCD base address
	str	r6, [r12,#0x08]                                                   @ R6: Unused
	str	r7, [r8]                                                          @ R7: Unused
	mov	r0, #0x11
	str	r0, [r8,#0x20]
	mov	r3, #0x33                                                         @ R3: 0x33
	beq	miu_coldboot
	str	r0, [lr,#0x104]
	b	miu_common
miu_coldboot:
	str	r3, [lr,#0x104]
	orr	r0, r3, #0x200
	str	r0, [lr,#0x104]
miu_wait3:
	ldr	r0, [lr,#0x104]
	tst	r0, #0x110000
	bne	miu_wait3
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
	orr	r1, r3, #0x300
	str	r1, [lr,#0x104]
miu_wait4:
	ldr	r0, [lr,#0x104]
	tst	r0, #0x110000
	bne	miu_wait4
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
	str	r1, [lr,#0x104]
miu_wait5:
	ldr	r0, [lr,#0x104]
	tst	r0, #0x110000
	bne	miu_wait5
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x110]
	orr	r0, r3, #0x100
	str	r0, [lr,#0x104]
miu_wait6:
	ldr	r0, [lr,#0x104]
	tst	r0, #0x110000
	bne	miu_wait6
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
	str	r5, [lr,#0x110]                                                   @ R5: Unused
	str	r1, [lr,#0x104]
miu_wait7:
	ldr	r0, [lr,#0x104]
	tst	r0, #0x110000
	bne	miu_wait7
	str	r3, [lr,#0x104]
	str	r3, [lr,#0x104]
miu_common:
	str	r3, [lr,#0x104]                                                   @ R3: Unused
	mov	r0, #0x40
	str	r0, [lr,#0x10c]
	ldr	r0, [lr,#0x100]
	orr	r0, r0, #0x9100000
	str	r0, [lr,#0x100]
	mov	r0, #0x19
	str	r0, [lr,#0x11c]
	mov	r0, #1
	str	r0, [lr,#0x120]
	orr	r1, r2, #0x1000                                                   @ R2: Unused
	str	r1, [lr,#0x108]
	str	r0, [lr,#0x08]
	mov	r1, #0x3e000000
	mov	r0, #0x1f
	str	r0, [r1,#0x08]

                           @ Block 7 (LCD) register map:
                           @ R0: Cmd/Data to be written / Scratchpad
                           @ R1: Scratchpad
                           @ R2: Scratchpad
                           @ R3: Scratchpad
                           @ R4: Scratchpad
                           @ R5: Scratchpad
                           @ R6: Unused
                           @ R7: Unused
                           @ R8: LCD base address
                           @ R9: 0
                           @ R10: I2C base address
                           @ R11: TIMER base address
                           @ R12: SYSCON base address
                           @ R13: LCD init script pointer / Scratchpad
                           @ R14: Return address / Scratchpad

.macro lcd_sequences
lcd_sequences_begin:
	.word	lcd_sequence_c4 - lcd_sequences_begin
	.word	lcd_sequence_d5 - lcd_sequences_begin
	.word	lcd_sequence_e6 - lcd_sequences_begin
	.word	lcd_sequence_b3 - lcd_sequences_begin
lcd_sequence_b3:
.byte 0x01, 0x11, 0xf8, 0x01, 0x13, 0x01, 0x29, 0x80
	.byte	0x01, 0x11
	.byte	0xf8
	.byte	0x02, 0xfe, 0x00
	.byte	0x02, 0xef, 0x80
	.byte	0x02, 0xc0, 0x0c
	.byte	0x02, 0xc1, 0x03
	.byte	0x03, 0xc2, 0x12, 0x00
	.byte	0x03, 0xc3, 0x12, 0x00
	.byte	0x03, 0xc4, 0x12, 0x00
	.byte	0x03, 0xc5, 0x3a, 0x3e
	.byte	0x03, 0xb1, 0x6a, 0x15
	.byte	0x03, 0xb2, 0x5f, 0x3f
	.byte	0x03, 0xb3, 0x5f, 0x3f
	.byte	0x02, 0xb4, 0x02
	.byte	0x03, 0xb6, 0x12, 0x02
	.byte	0x02, 0x35, 0x00
	.byte	0x02, 0x26, 0x10
	.byte	0x0c, 0xe0, 0x0f, 0x42, 0x24, 0x01, 0x00, 0x02, 0xa6, 0x98, 0x05, 0x04, 0x15
	.byte	0x0c, 0xe1, 0x00, 0x21, 0x44, 0x02, 0x0f, 0x05, 0x89, 0x6a, 0x02, 0x15, 0x04
	.byte	0x0c, 0xe2, 0x7e, 0x04, 0x43, 0x40, 0x00, 0x02, 0x13, 0x00, 0x00, 0x01, 0x0b
	.byte	0x0c, 0xe3, 0x40, 0x40, 0x03, 0x74, 0x0e, 0x00, 0x00, 0x31, 0x02, 0x0b, 0x01
	.byte	0x0c, 0xe4, 0x5a, 0x43, 0x67, 0x56, 0x00, 0x02, 0x67, 0x72, 0x00, 0x05, 0x12
	.byte	0x0c, 0xe5, 0x50, 0x66, 0x47, 0x53, 0x0a, 0x00, 0x27, 0x76, 0x02, 0x12, 0x05
	.byte	0x02, 0x3a, 0x06
	.byte	0x01, 0x13
	.byte	0x01, 0x29
	.byte	0x80
lcd_sequence_c4:
	.byte	0x01, 0x01
	.byte	0x85
	.byte	0x02, 0xc0, 0x00
	.byte	0x02, 0xc1, 0x03
	.byte	0x02, 0xc2, 0x34
	.byte	0x03, 0xc3, 0x72, 0x03
	.byte	0x03, 0xc4, 0x73, 0x03
	.byte	0x03, 0xc5, 0x3c, 0x3c
	.byte	0x02, 0xfe, 0x00
	.byte	0x03, 0xb1, 0x6a, 0x15
	.byte	0x03, 0xb2, 0x6a, 0x15
	.byte	0x03, 0xb3, 0x6a, 0x15
	.byte	0x02, 0xb4, 0x02
	.byte	0x03, 0xb6, 0x12, 0x02
	.byte	0x02, 0x35, 0x00
	.byte	0x02, 0x26, 0x10
	.byte	0x0c, 0xe0, 0x77, 0x52, 0x76, 0x53, 0x03, 0x03, 0x57, 0x42, 0x10, 0x18, 0x09
	.byte	0x0c, 0xe1, 0x0d, 0x00, 0x23, 0x66, 0x0f, 0x15, 0x4d, 0x85, 0x08, 0x02, 0x10
	.byte	0x0c, 0xe2, 0x39, 0x60, 0x77, 0x05, 0x03, 0x07, 0x96, 0x64, 0x0d, 0x1a, 0x0a
	.byte	0x0c, 0xe3, 0x3f, 0x10, 0x16, 0x44, 0x0e, 0x04, 0x6c, 0x44, 0x04, 0x03, 0x0b
	.byte	0x0c, 0xe4, 0x00, 0x61, 0x77, 0x04, 0x02, 0x04, 0x72, 0x32, 0x09, 0x19, 0x06
	.byte	0x0c, 0xe5, 0x4f, 0x42, 0x27, 0x67, 0x0f, 0x02, 0x26, 0x33, 0x01, 0x03, 0x09
	.byte	0x02, 0x36, 0x00
	.byte	0x01, 0x11
	.byte	0x01, 0x29
	.byte	0x80
lcd_sequence_d5:
.byte 0x01, 0x01, 0x85, 0x01, 0x11, 0x01, 0x29, 0x80
	.byte	0x02, 0xfe, 0x00
	.byte	0x02, 0xc0, 0x01
	.byte	0x02, 0xc1, 0x01
	.byte	0x03, 0xc2, 0x03, 0x00
	.byte	0x03, 0xc3, 0x01, 0x00
	.byte	0x03, 0xc4, 0x03, 0x00
	.byte	0x03, 0xc5, 0x34, 0x34
	.byte	0x02, 0xc7, 0x00
	.byte	0x03, 0xb1, 0x6d, 0x15
	.byte	0x03, 0xb2, 0x6d, 0x15
	.byte	0x03, 0xb3, 0x6d, 0x15
	.byte	0x02, 0xb4, 0x03
	.byte	0x03, 0xb6, 0x11, 0x02
	.byte	0x02, 0x35, 0x00
	.byte	0x02, 0x26, 0x10
	.byte	0x0c, 0xe0, 0x23, 0x42, 0x20, 0x42, 0x0e, 0x01, 0xf5, 0xeb, 0x1e, 0x05, 0x18
	.byte	0x0c, 0xe1, 0x5f, 0x22, 0x36, 0x21, 0x03, 0x1e, 0xfe, 0x7b, 0x02, 0x07, 0x18
	.byte	0x0c, 0xe2, 0x5f, 0x34, 0x53, 0x77, 0x0a, 0x00, 0x70, 0xf4, 0x14, 0x06, 0x0f
	.byte	0x0c, 0xe3, 0x0f, 0x23, 0x31, 0x54, 0x0f, 0x0b, 0x8e, 0x08, 0x00, 0x05, 0x15
	.byte	0x0c, 0xe4, 0x5f, 0x33, 0x42, 0x14, 0x0e, 0x04, 0xa6, 0xf7, 0x0e, 0x00, 0x14
	.byte	0x0c, 0xe5, 0x0c, 0x43, 0x44, 0x44, 0x0d, 0x0d, 0x7f, 0x39, 0x03, 0x02, 0x10
	.byte	0x02, 0x3a, 0x66
	.byte	0x02, 0x36, 0x00
	.byte	0x01, 0x11
	.byte	0x01, 0x29
	.byte	0x80
lcd_sequence_e6:
.byte 0x01, 0x11, 0xf8, 0x01, 0x13, 0x01, 0x29, 0x80
	.byte	0x01, 0x11
	.byte	0xf8
	.byte	0x02, 0xfe, 0x00
	.byte	0x02, 0xef, 0x80
	.byte	0x02, 0xc0, 0x13
	.byte	0x02, 0xc1, 0x03
	.byte	0x03, 0xc2, 0x12, 0x00
	.byte	0x03, 0xc3, 0x12, 0x00
	.byte	0x03, 0xc4, 0x12, 0x00
	.byte	0x03, 0xc5, 0x2a, 0x3c
	.byte	0x03, 0xb1, 0x6a, 0x15
	.byte	0x03, 0xb2, 0x5f, 0x3f
	.byte	0x03, 0xb3, 0x5f, 0x3f
	.byte	0x02, 0xb4, 0x02
	.byte	0x03, 0xb6, 0x12, 0x02
	.byte	0x02, 0x35, 0x00
	.byte	0x02, 0x26, 0x10
	.byte	0x0c, 0xe0, 0x0f, 0x53, 0x45, 0x07, 0x00, 0x00, 0xb9, 0xf6, 0x08, 0x04, 0x18
	.byte	0x0c, 0xe1, 0x00, 0x47, 0x55, 0x03, 0x0f, 0x08, 0x6f, 0x9b, 0x00, 0x18, 0x04
	.byte	0x0c, 0xe2, 0x7e, 0x03, 0x54, 0x75, 0x00, 0x00, 0x3a, 0x52, 0x03, 0x02, 0x10
	.byte	0x0c, 0xe3, 0x70, 0x55, 0x04, 0x73, 0x0e, 0x03, 0x25, 0xa3, 0x00, 0x10, 0x02
	.byte	0x0c, 0xe4, 0x1a, 0x72, 0x33, 0x76, 0x00, 0x00, 0xeb, 0x97, 0x03, 0x05, 0x17
	.byte	0x0c, 0xe5, 0x70, 0x36, 0x73, 0x12, 0x0a, 0x03, 0x79, 0xbe, 0x00, 0x17, 0x05
	.byte	0x02, 0x3a, 0x06
	.byte	0x01, 0x13
	.byte	0x01, 0x29
	.byte	0x80
lcd_sequences_end:
.endm

	mov	r0, #4
	bl	sendlcdc
	bl	readlcd
	bl	readlcd
	bl	readlcd
	and	r0, r0, #3
	ldr	r0, [sp,r0,lsl#2]
	add	sp, sp, r0
lcdloop:
	ldrb	r1, [sp], #1
	tst	r1, #0x80
	beq	lcddata
	bics	r1, r1, #0x80
	beq	lcddone
	mov	r0, r1,lsl#10
	bl	udelay
	b	lcdloop
lcddata:
	ldrb	r0, [sp], #1
	bl	sendlcdc
lcdbyte:
	subs	r1, r1, #1
	beq	lcdloop
	ldrb	r0, [sp], #1
	bl	sendlcdd
	b	lcdbyte

values1:
	block0_constpool
	block1_constpool
	gpio_initdata
	pmu_batch_1
	pmu_batch_2
	pmu_batch_3
	pmu_batch_4
	pmu_batch_5
	block6_constpool
	lcd_sequences
	
	.align	1
	.code	16
thumb_nrv2e_d8:
	mov r7,r2
        mov r4,#1; neg r5,r4 @ r5= -1 initial condition
        lsl r4,#31 @ 1<<31: refill next time
        mov r6,#5
        lsl r6,#8 @ 0x500 @ nrv2e M2_MAX_OFFSET
        b top_n2e

nrv2e_done:
	blx execfirmware

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
	.code	32

font:
	.byte	0, 0, 0, 0, 0
	.byte	0, 0, 95, 0, 0
	.byte	0, 7, 0, 7, 0
	.byte	20, 127, 20, 127, 20
	.byte	36, 42, 127, 42, 18
	.byte	35, 19, 8, 100, 98
	.byte	54, 73, 85, 34, 80
	.byte	5, 3, 0, 0, 0
	.byte	28, 34, 65, 0, 0
	.byte	0, 0, 65, 34, 28
	.byte	20, 8, 62, 8, 20
	.byte	8, 8, 62, 8, 8
	.byte	0, -96, 96, 0, 0
	.byte	8, 8, 8, 8, 8
	.byte	0, 96, 96, 0, 0
	.byte	32, 16, 8, 4, 2
	.byte	62, 81, 73, 69, 62
	.byte	0, 66, 127, 64, 0
	.byte	66, 97, 81, 73, 70
	.byte	33, 65, 69, 75, 49
	.byte	24, 20, 18, 127, 16
	.byte	39, 69, 69, 69, 57
	.byte	60, 74, 73, 73, 48
	.byte	1, 113, 9, 5, 3
	.byte	54, 73, 73, 73, 54
	.byte	6, 73, 73, 41, 30
	.byte	0, 54, 54, 0, 0
	.byte	0, 86, 54, 0, 0
	.byte	8, 20, 34, 65, 0
	.byte	20, 20, 20, 20, 20
	.byte	0, 65, 34, 20, 8
	.byte	2, 1, 81, 9, 6
	.byte	50, 73, 121, 65, 62
	.byte	124, 18, 17, 18, 124
	.byte	127, 73, 73, 73, 62
	.byte	62, 65, 65, 65, 34
	.byte	127, 65, 65, 34, 28
	.byte	127, 73, 73, 73, 65
	.byte	127, 9, 9, 9, 1
	.byte	62, 65, 73, 73, 58
	.byte	127, 8, 8, 8, 127
	.byte	0, 65, 127, 65, 0
	.byte	32, 64, 65, 63, 1
	.byte	127, 8, 20, 34, 65
	.byte	127, 64, 64, 64, 64
	.byte	127, 2, 12, 2, 127
	.byte	127, 4, 8, 16, 127
	.byte	62, 65, 65, 65, 62
	.byte	127, 9, 9, 9, 6
	.byte	62, 65, 81, 33, 94
	.byte	127, 9, 25, 41, 70
	.byte	38, 73, 73, 73, 50
	.byte	1, 1, 127, 1, 1
	.byte	63, 64, 64, 64, 63
	.byte	31, 32, 64, 32, 31
	.byte	127, 32, 24, 32, 127
	.byte	99, 20, 8, 20, 99
	.byte	3, 4, 120, 4, 3
	.byte	97, 81, 73, 69, 67
	.byte	0, 127, 65, 65, 0
	.byte	2, 4, 8, 16, 32
	.byte	0, 65, 65, 127, 0
	.byte	4, 2, 1, 2, 4
	.byte	64, 64, 64, 64, 64
	.byte	1, 2, 4, 0, 0
	.byte	32, 84, 84, 84, 120
	.byte	127, 68, 68, 68, 56
	.byte	56, 68, 68, 68, 40
	.byte	56, 68, 68, 68, 127
	.byte	56, 84, 84, 84, 24
	.byte	8, 126, 9, 1, 2
	.byte	8, 84, 84, 84, 60
	.byte	127, 4, 4, 4, 120
	.byte	0, 68, 125, 64, 0
	.byte	32, 64, 64, 61, 0
	.byte	127, 16, 40, 68, 0
	.byte	0, 65, 127, 64, 0
	.byte	124, 4, 24, 4, 120
	.byte	124, 8, 4, 4, 120
	.byte	56, 68, 68, 68, 56
	.byte	124, 20, 20, 20, 24
	.byte	8, 20, 20, 20, 124
	.byte	124, 8, 4, 4, 8
	.byte	72, 84, 84, 84, 32
	.byte	4, 63, 68, 64, 32
	.byte	60, 64, 64, 32, 124
	.byte	28, 32, 64, 32, 28
	.byte	60, 64, 56, 64, 60
	.byte	68, 40, 16, 40, 68
	.byte	12, 80, 80, 80, 60
	.byte	68, 100, 84, 76, 68
	.byte	0, 8, 54, 65, 0
	.byte	0, 0, 119, 0, 0
	.byte	0, 65, 54, 8, 0
	.byte	2, 1, 2, 4, 2

errormessage:
	.ascii "File not found!\0"

bootfilename:
	.ascii "emcore  "

text:
	.ascii "emCORE Loader v"
	.ascii VERSION
	.ascii " r"
	.ascii VERSION_SVN
	.ascii "\0"
	.ascii "Loading emCORE...\0"

	.align	2

execfirmware:
	mcr	p15, 0, r9,c7,c14,0 @ clean & invalidate data cache
	bx	r7

                           @ rendertext register map:
                           @ R0: Pointer to string, will be incremented
                           @ R1: Framebuffer address, will be incremented
                           @ R2: Color
                           @ R3: Trashed
                           @ R4: Trashed
                           @ R4: Trashed
                           @ R9: Set to 0
                           @ R14: Return address

rendertext:
	ldrb	r3, [r0], #1
	cmp	r3, #0
	moveq	pc, lr
	adr	r5, font
	sub	r3, r3, #0x20
	cmp	r3, #0x5f
	addcc	r5, r3,lsl#2
	addcc	r5, r3
	mov	r3, #5
rendertext_col:
	mov	r4, r1
	ldrb	r9, [r5], #1
rendertext_row:
	tst	r9, #1
	strneh	r2, [r4]
	add	r4, r4, #480
	movs	r9, r9,lsr#1
	bne	rendertext_row
	add	r1, r1, #2
	subs	r3, r3, #1
	bne	rendertext_col
	add	r1, r1, #2
	b	rendertext

lcd_mode:
	.word	0x41100db8


                           @ Block 8 (DRAW) register map:
                           @ R0: Scratchpad
                           @ R1: Scratchpad
                           @ R2: Scratchpad
                           @ R3: Scratchpad
                           @ R4: Scratchpad
                           @ R5: Scratchpad
                           @ R6: Unused
                           @ R7: Unused
                           @ R8: LCD base address
                           @ R9: 0
                           @ R10: I2C base address
                           @ R11: TIMER base address
                           @ R12: SYSCON base address
                           @ R13: Scratchpad
                           @ R14: Return address / Scratchpad

lcddone:
	ldr	r0, lcd_mode
	str	r0, [r8]
	mov	r1, #0x22000000
	orr	r1, r1, #0x6800
	add	r2, r1, #0x25800
	mov	r0, #-1
fillbuff:
	str	r0, [r2,#-4]!
	cmp	r1, r2
	bne	fillbuff
	mov	sp, r1
	adr	r0, text
	mov	r2, #0
	bl	rendertext
	add	r1, sp, #0x1e00
printerror:
	bl	rendertext
	mcr	p15, 0, r9,c7,c10,0 @ clean data cache
	mcr	p15, 0, r9,c7,c10,4 @ drain write buffer
	mov	r0, #0x2a
	bl	sendlcdc
	mov	r0, #0
	bl	sendlcdd
	mov	r0, #0xef
	bl	sendlcdd
	mov	r0, #0x2b
	bl	sendlcdc
	mov	r0, #0
	bl	sendlcdd
	mov	r0, #0x200
	orr	r0, r0, #0x3f
	bl	sendlcdd
	mov	r0, #0x2c
	bl	sendlcdc
blit:
	sub	lr, r8, #0x00100000
	mov	sp, #1
	str	sp, [lr,#0x30]
	mov	sp, #0x22000000
	orr	sp, sp, #0x2d000
	add	lr, lr, #0x100
	mov	r4, #0x8800
	orr	r4, r4, #0xc1
	ldmia	sp, {r0-r3}
	stmia	lr, {r0-r4}
@	tst	r5, #0xff
@failed:
@	bne	failed
	mov	r0, #0x30
	mov	r1, #120
	bl	pmuwrite
	mov	r0, #0x31
	mov	r1, #1
	bl	pmuwrite


hang: b hang

                           @ udelay register map:
                           @ R0: Microseconds
                           @ R1: Trashed
                           @ R11: TIMER base address
                           @ R14: Return address

udelay:
	ldr	r1, [r11,#0xb4]
	add	r0, r0, r1
udelayloop:
	ldr	r1, [r11,#0xb4]
	cmp	r1, r0
	bmi	udelayloop
	mov	pc, lr

                           @ i2cwaitrdy register map:
                           @ R9: Set to 0
                           @ R10: I2C base address
                           @ R14: Return address

i2cwaitrdy:
	ldr	r9, [r10,#0x10]
	cmp	r9, #0
	bne	i2cwaitrdy
	mov	pc, lr

                           @ i2cwait register map:
                           @ R3: Set to 0x10
                           @ R10: I2C base address
                           @ R14: Return address

i2cwait:
	ldr	r3, [r10]
	ands	r3, #0x10
	beq	i2cwait
	mov	pc, lr

                           @ pmuwrite register map:
                           @ R0: Address
                           @ R1: Data
                           @ R2: Set to 0xb7
                           @ R3: Set to 0x10
                           @ R4: Return address backup
                           @ R10: I2C base address
                           @ R14: Return address / Scratchpad

pmuwrite:
	mov	r4, lr
	mov	lr, #0xe6
	str	lr, [r10,#0x0c]
	mov	lr, #0xf0
	str	lr, [r10,#0x04]
	mov	r2, #0xb7
	str	r2, [r10]
	bl	i2cwait
	str	r0, [r10,#0x0c]
	str	r2, [r10]
	bl	i2cwait
	str	r1, [r10,#0x0c]
	str	r2, [r10]
	bl	i2cwait
	mov	lr, #0xd0
	str	lr, [r10,#0x04]
	str	r2, [r10]
pmuwrite_wait:
	ldr	lr, [r10,#0x04]
	tst	lr, #0x20
	bne	pmuwrite_wait
	mov	pc, r4

                           @ pmuread register map:
                           @ R0: Address
                           @ R1: Data
                           @ R2: Set to 0xb7
                           @ R3: Set to 0x10
                           @ R4: Return address backup
                           @ R10: I2C base address
                           @ R14: Return address / Scratchpad

pmuread:
	mov	r4, lr
	mov	lr, #0xe6
	str	lr, [r10,#0x0c]
	mov	lr, #0xf0
	str	lr, [r10,#0x04]
	mov	r2, #0xb7
	str	r2, [r10]
	bl	i2cwait
	str	r0, [r10,#0x0c]
	str	r2, [r10]
	bl	i2cwait
	mov	r1, #0xe7
	str	r1, [r10,#0x0c]
	mov	r1, #0xb0
	str	r1, [r10,#0x04]
	str	r2, [r10]
	bl	i2cwait
	mov	r1, #0x37
	str	r1, [r10]
	bl	i2cwait
	ldr	r1, [r10,#0x0c]
	mov	lr, #0x90
	str	lr, [r10,#0x04]
	str	r2, [r10]
pmuread_wait:
	ldr	lr, [r10,#0x04]
	tst	lr, #0x20
	bne	pmuread_wait
	mov	pc, r4

                           @ pmubatch register map:
                           @ R0: Scratchpad
                           @ R1: Scratchpad
                           @ R2: Set to 0xb7
                           @ R3: Set to 0x10
                           @ R4: Inner return address backup
                           @ R7: Outer return address backup
                           @ R9: Number of address-data pairs to be sent (must be >0), will be reset to 0
                           @ R10: I2C base address
                           @ R13: Address-data pair list pointer (will be incremented)
                           @ R14: Return address / Scratchpad

pmubatch:
	mov	r7, lr
pmubatch_loop:
	ldrb	r0, [sp], #1
	ldrb	r1, [sp], #1
	bl	pmuwrite
	subs	r9, r9, #1
	bne	pmubatch_loop
	mov	pc, r7

                           @ sendlcdc register map:
                           @ R0: Command to be sent
                           @ R8: LCD base address
                           @ R9: Will be set to 0
                           @ R14: Return address

sendlcdc:
	ldr	r9, [r8,#0x1c]
	ands	r9, r9, #0x10
	bne	sendlcdc
	str	r0, [r8,#0x04]
	mov	pc, lr

                           @ sendlcdd register map:
                           @ R0: Data to be sent
                           @ R8: LCD base address
                           @ R9: Will be set to 0
                           @ R14: Return address

sendlcdd:
	ldr	r9, [r8,#0x1c]
	ands	r9, r9, #0x10
	bne	sendlcdd
	str	r0, [r8,#0x40]
	mov	pc, lr
	
                           @ readlcd register map:
                           @ R0: Result data
                           @ R8: LCD base address
                           @ R14: Return address

readlcd:
	ldr	r0, [r8,#0x1c]
	tst	r0, #2
	beq	readlcd
	str	r0, [r8,#0x10]
readlcd_wait:
	ldr	r0, [r8,#0x1c]
	tst	r0, #0x1
	beq	readlcd_wait
	ldr	r0, [r8,#0x14]
	mov	r0, r0,lsr#1
	mov	pc, lr
