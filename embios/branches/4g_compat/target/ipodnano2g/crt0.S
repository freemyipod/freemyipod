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


.section .intvect,"ax",%progbits
	ldr pc, =reset_handler
	ldr pc, =undef_instr_handler
	ldr pc, =syscall_handler
	ldr pc, =prefetch_abort_handler
	ldr pc, =data_abort_handler
	ldr pc, =reserved_handler
	ldr pc, =irq_handler
	ldr pc, =fiq_handler
.ltorg


.section .initcode,"ax",%progbits
.global _start
_start:
	ldr	r0, =_sramsource
	ldr	r1, =_sramstart
	ldr	r2, =_sramend
.copysram:
	cmp	r2, r1
	ldrhi	r3, [r0], #4
	strhi	r3, [r1], #4
	bhi	.copysram
	ldr	r0, =_sdramsource
	ldr	r1, =_sdramstart
	ldr	r2, =_sdramend
.copysdram:
	cmp	r2, r1
	ldrhi	r3, [r0], #4
	strhi	r3, [r1], #4
	bhi	.copysdram
	ldr	r0, =_initbssstart
	ldr	r1, =_initbssend
	mov	r2, #0
.clearinitbss:
	cmp	r1, r0
	strhi	r2, [r0], #4
	bhi	.clearinitbss
	ldr	r0, =_ibssstart
	ldr	r1, =_ibssend
.clearibss:
	cmp	r1, r0
	strhi	r2, [r0], #4
	bhi	.clearibss
	ldr	r0, =_bssstart
	ldr	r1, =_bssend
.clearbss:
	cmp	r1, r0
	strhi	r2, [r0], #4
	bhi	.clearbss
	ldr	r1, =0x38200000
	ldr	r0, [r1]
	orr	r0, r0, #1
	bic	r0, r0, #0x10000
	str	r0, [r1]
	mov	r0, #0
	mcr	p15, 0, r0,c7,c5,0
	msr	cpsr_c, #0xd2
	ldr	sp, =_irqstackend
	msr	cpsr_c, #0xd7
	ldr	sp, =_abortstackend
	msr	cpsr_c, #0xdb
	ldr	sp, =_abortstackend
	msr	cpsr_c, #0xd3
	ldr	sp, =_initstackend
	bl	init
	b	main
.ltorg


.section .icode, "ax", %progbits
.align 2
.global reset
.global hang
.type reset, %function
.type hang, %function
reset:
	msr	cpsr_c, #0xd3
	mov	r0, #0x110000
	add	r0, r0, #0xff
	add	r1, r0, #0xa00
	mov	r2, #0x3c800000
	str	r1, [r2]
	mov	r1, #0xff0
	str	r1, [r2,#4]
	str	r0, [r2]
hang:
	b	hang
.size reset, .-reset
.size hang, .-hang

.type reset_handler, %function
reset_handler:
	adr	r0, reset_text
	b	panic
reset_text:
	.ascii	"Hit reset vector!\0"
.size reset_handler, .-reset_handler

.type undef_instr_handler, %function
undef_instr_handler:
	adr	r0, undef_instr_text
	sub	r0, lr, #4
	b	panicf
.size undef_instr_handler, .-undef_instr_handler

.type prefetch_abort_handler, %function
prefetch_abort_handler:
	adr	r0, prefetch_abort_text
	sub	r0, lr, #4
	b	panicf
.size prefetch_abort_handler, .-prefetch_abort_handler

.type data_abort_handler, %function
data_abort_handler:
	adr	r0, data_abort_text
	sub	r0, lr, #4
	b	panicf
.size data_abort_handler, .-data_abort_handler

.type reserved_handler, %function
reserved_handler:
	adr	r0, reserved_text
	b	panic
.size reserved_handler, .-reserved_handler

.type fiq_handler, %function
fiq_handler:
	adr	r0, fiq_text
	b	panic
.size fiq_handler, .-fiq_handler

.type irq_handler, %function
irq_handler:
	adr	r0, irq_text
	b	panic
.size irq_handler, .-irq_handler

.type syscall_handler, %function
syscall_handler:
	adr	r0, syscall_text
	b	panic
.size syscall_handler, .-syscall_handler

undef_instr_text:
	.ascii	"Undefined instruction at %08X!\0"

prefetch_abort_text:
	.ascii	"Prefetch abort at %08X!\0"

data_abort_text:
	.ascii	"Data abort at %08X!\0"

reserved_text:
	.ascii	"Hit reserved exception handler!\0"

fiq_text:
	.ascii	"Unhandled FIQ!\0"

irq_text:
	.ascii	"Unhandled IRQ!\0"

syscall_text:
	.ascii	"Unhandled syscall!\0"
