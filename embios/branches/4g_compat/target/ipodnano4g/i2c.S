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
	add	r12, r12, #0x00600000
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
	stmfd	sp!, {r0,r4,lr}    @ R0 = slave id, R1 = I2C address, R2 = data, R3 = length
	mov	r12, #0x3C000000       @ R12 = I2C base address
	add	r12, r12, #0x00600000  @ DIFF!! something needs to be done about the different busses
	mov	r4, #0
	str	r4, [r12,#0x08]        @ store 0 to the i2c address + 0x8
	str	r0, [r12,#0x0c]        @ write the slave id to the i2c handler
	mov	r4, #0xf0
	str	r4, [r12,#0x04]        @ write 0xF0 to the i2c handler (idk why)
	mov	r4, #0xf3
	str	r4, [r12]              @ write 0xF3 to the i2c handler (apparently a finished signal)
	bl	i2cwait
	str	r1, [r12,#0x0c]        @ write the i2c address to the i2c handler
	str	r4, [r12]              @ write 0xF3 to the i2c handler (apparently a finished signal)
	bl	i2cwait
	ldr	r0, [sp]               @ DIFF!! why is it stack pointer here?
	orr	r0, r0, #1             @ find the least significant bit of the slave id
	str	r0, [r12,#0x0c]        @ write it to the i2c handler
	mov	r0, #0xb0
	str	r0, [r12,#0x04]        @ write 0xB0 to the i2c handler (idk why)
	str	r4, [r12]              @ write 0xF3 to the i2c handler (apparently a finished signal)
	bl	i2cwait
i2crecv_read:
	subs	r3, r3, #1         @ decrement the bytes left
	moveq	r4, #0x73          @ set R6 to 0x73 if the bytes left is now 0
	str	r4, [r12]
	bl	i2cwait
	ldr	r0, [r12,#0x0c]        @ read a byte into R0
	movs	r2, r2
	strne	r0, [r2], #1       @ write it to our buffer
	movs	r3, r3
	bne	i2crecv_read           @ repeat if there are still bytes left
	mov	r1, #0x90
	str	r1, [r12,#0x04]        @ write 0x90 to the i2c handler
	mov	r1, #0xf3
	str	r1, [r12]              @ write 0xF3 to the i2c handler (apparently a finished signal)
i2crecv_wait:
	ldr	r1, [r12,#0x04]
	tst	r1, #0x20
	bne	i2crecv_wait           @ wait until [R12,#0x04] does not equal 0x20
	ldmfd	sp!, {r1,r4,pc}
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
