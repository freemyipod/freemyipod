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


.section .icode.i2csend, "ax", %progbits
.align 2
.global i2csendbyte
.global i2csend
.type i2csendbyte, %function
.type i2csend, %function
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
	mov	r4, #0xf3
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
.size i2csendbyte, .-i2csendbyte
.size i2csend, .-i2csend


.section .icode.i2crecv, "ax", %progbits
.align 2
.global i2crecvbyte
.global i2crecv
.type i2crecvbyte, %function
.type i2crecv, %function
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
	mov	r4, #0xf3
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
	moveq	r4, #0x73
	str	r4, [r12]
	bl	i2cwait
	ldr	r0, [r12,#0x0c]
	movs	r2, r2
	strne	r0, [r2], #1
	movs	r3, r3
	bne	i2crecv_read
	mov	r1, #0x90
	str	r1, [r12,#0x04]
	mov	r1, #0xf3
	str	r1, [r12]
i2crecv_wait:
	ldr	r1, [r12,#0x04]
	tst	r1, #0x20
	bne	i2crecv_wait
	ldmfd	sp!, {r0,r4,pc}
.size i2crecvbyte, .-i2crecvbyte
.size i2crecv, .-i2crecv


.section .icode.i2cwait, "ax", %progbits
.align 2
.global i2cwait
.type i2cwait, %function
i2cwait:
	ldr	r0, [r12]
	tst	r0, #0x10
	beq	i2cwait
	mov	pc, lr
.size i2cwait, .-i2cwait
