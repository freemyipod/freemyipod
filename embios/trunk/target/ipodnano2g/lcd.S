@
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
@    You should have received a copy of the GNU General Public License
@    along with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
@
@


.section .icode.displaylcd, "ax", %progbits
.align 2
.global displaylcd
.type displaylcd, %function
displaylcd:
	stmfd	sp!, {r0,r1,r4,lr}
	bl	displaylcd_sync
	bl	displaylcd_detectlcd
	sub	r12, r12, #0x04900000
	cmp	r0, #2
	bne	displaylcd_othertypes
	mov	r0, #0x50
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x51
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp,#0x04]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x52
	bl	displaylcd_sendlcd2c
	mov	r0, r2
	bl	displaylcd_sendlcd2d
	mov	r0, #0x53
	bl	displaylcd_sendlcd2c
	mov	r0, r3
	bl	displaylcd_sendlcd2d
	mov	r0, #0x20
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x21
	bl	displaylcd_sendlcd2c
	mov	r0, r2
	bl	displaylcd_sendlcd2d
	mov	r0, #0x22
	bl	displaylcd_sendlcd2c
	b	displaylcd_blit
displaylcd_othertypes:
	mov	r0, #0x2a
	bl	displaylcd_sendlcdc
	ldr	r0, [sp]
	bl	displaylcd_sendlcd2d
	ldr	r0, [sp,#0x04]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x2b
	bl	displaylcd_sendlcdc
	mov	r0, r2
	bl	displaylcd_sendlcd2d
	mov	r0, r3
	bl	displaylcd_sendlcd2d
	mov	r0, #0x2c
	bl	displaylcd_sendlcdc
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
	bl	displaylcd_sendlcd2d
	subs	r2, r2, #1
	bne	displaylcd_pixel
	ldmfd	sp!, {r0,r1,r4,pc}
displaylcd_dma:
	mov	r4, #0x38000000
	add	r4, r4, #0x400000
	mov	r0, #0x20000000
	orr	r0, r0, #0x180000
	str	r0, [r4,#0x104]
	str	r1, [r4,#0x100]
	mov	r0, r2,lsr#1
	sub	r0, r0, #1
	str	r0, [r4,#0x108]
	bl	clean_dcache
	mov	r0, #4
	str	r0, [r4,#0x114]
	ldmfd	sp!, {r0,r1,r4,pc}

displaylcd_sendlcd2c:
	ldr	r4, [r12,#0x1c]
	ands	r4, r4, #0x10
	bne	displaylcd_sendlcd2c
	mov	r4, r0,lsr#8
	str	r4, [r12,#0x04]
	and	r0, r0, #0xff
displaylcd_sendlcdc:
	ldr	r4, [r12,#0x1c]
	ands	r4, r4, #0x10
	bne	displaylcd_sendlcdc
	str	r0, [r12,#0x04]
	mov	pc, lr
displaylcd_sendlcd2d:
	ldr	r4, [r12,#0x1c]
	ands	r4, r4, #0x10
	bne	displaylcd_sendlcd2d
	mov	r4, r0,lsr#8
	str	r4, [r12,#0x40]
	and	r0, r0, #0xff
displaylcd_sendlcdd:
	ldr	r4, [r12,#0x1c]
	ands	r4, r4, #0x10
	bne	displaylcd_sendlcdd
	str	r0, [r12,#0x40]
	mov	pc, lr

displaylcd_detectlcd:
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
.size displaylcd, .-displaylcd


.section .icode.displaylcd_sync, "ax", %progbits
.align 2
.global displaylcd_sync
.type displaylcd_sync, %function
displaylcd_sync:
	mov	r1, #0x38000000
	add	r1, r1, #0x400000
displaylcd_sync_wait:
	ldr	r0, [r1,#0x184]
	tst	r0, #0x40000
	bne	displaylcd_sync_wait
	mov	pc, lr
.size displaylcd_sync, .-displaylcd_sync
