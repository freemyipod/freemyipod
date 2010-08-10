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


.section .initcode.lcd_init, "ax", %progbits
.align 2
.global lcd_init
.type lcd_init, %function
lcd_init:
	mov	pc, lr
.size lcd_init, .-lcd_init

.section .icode.displaylcd, "ax", %progbits
.align 2
.global displaylcd
.type displaylcd, %function
displaylcd:
	stmfd	sp!, {r0,r1,r4,lr}
	mov	r12, #0x38000000
	orr	r12, r12, #0x300000
	mov	r0, #0x2a
	bl	displaylcd_sendlcdc
	ldr	r0, [sp]
	bl	displaylcd_sendlcdd
	ldr	r0, [sp,#0x04]
	bl	displaylcd_sendlcdd
	mov	r0, #0x2b
	bl	displaylcd_sendlcdc
	mov	r0, r2
	tst	r0, #0x100
	eorne	r0, #0x300
	bl	displaylcd_sendlcdd
	mov	r0, r3
	tst	r0, #0x100
	eorne	r0, #0x300
	bl	displaylcd_sendlcdd
	mov	r0, #0x2c
	bl	displaylcd_sendlcdc
	ldmia	sp, {r0,r1}
	sub	r1, r0
	add	r1, r1, #1
	sub	r3, r2
	add	r3, r3, #1
	mul	r2, r1, r3
	ldr	r1, [sp,#0x10]
	cmp	r1, #0x40000000
	bne	displaylcd_framebuf
displaylcd_color:
	ldr	r0, [sp,#0x14]
	bl	displaylcd_sendlcdd
	subs	r2, r2, #1
	bne	displaylcd_color
	ldmfd	sp!, {r4-r6,pc}
displaylcd_framebuf:
	ldrh	r0, [r1], #2
	bl	displaylcd_sendlcdd
	subs	r2, r2, #1
	bne	displaylcd_framebuf
	ldmfd	sp!, {r0,r1,r4,pc}

displaylcd_sendlcdc:
	ldrh	r4, [r12,#0x1c]
	tst	r4, #0x10
	bne	displaylcd_sendlcdc
	strh	r0, [r12,#0x04]
	mov	pc, lr
displaylcd_sendlcdd:
	ldrh	r4, [r12,#0x1c]
	tst	r4, #0x10
	bne	displaylcd_sendlcdd
	strh	r0, [r12,#0x40]
	mov	pc, lr
.size displaylcd, .-displaylcd

.section .icode.displaylcd_sync, "ax", %progbits
.align 2
.global displaylcd_sync
.type displaylcd_sync, %function
displaylcd_sync:
	mov	pc, lr
.size displaylcd_sync, .-displaylcd_sync

.section .icode.displaylcd_busy, "ax", %progbits
.align 2
.global displaylcd_busy
.type displaylcd_busy, %function
displaylcd_busy:
	mov	r0, #0
	mov	pc, lr
.size displaylcd_busy, .-displaylcd_busy

.section .icode.displaylcd_safe, "ax", %progbits
.align 2
.global displaylcd_safe
.type displaylcd_safe, %function
displaylcd_safe:
	mov	r0, #1
	mov	pc, lr
.size displaylcd_safe, .-displaylcd_safe

