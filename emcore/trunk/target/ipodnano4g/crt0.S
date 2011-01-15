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


.section .inithead,"ax",%progbits
.global __start
__start:
	b	_start

.section .initcode,"ax",%progbits
.global _start
_start:
	ldr	r0, =0x00450878
	mcr	p15, 0, r0,c1,c0,0
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
	add	r1, r1, #0x00c00000
	add	r2, r1, #0x00001000
	add	r3, r1, #0x00002000
	sub	r4, r0, #1
	str	r4, [r1,#0x14]
	str	r4, [r2,#0x14]
	str	r4, [r1,#0xf00]
	str	r4, [r2,#0xf00]
	str	r4, [r3,#0x08]
	str	r4, [r3,#0x0c]
	str	r0, [r1,#0x14]
	str	r0, [r2,#0x14]
	mov	r0, #0
	ldr	r1, =0x3c500000
	str	r0, [r1,#0x48]
	str	r0, [r1,#0x4c]
	msr	cpsr_c, #0xd2
	ldr	sp, =_irqstackend
	msr	cpsr_c, #0xd7
	ldr	sp, =_abortstackend
	msr	cpsr_c, #0xdb
	ldr	sp, =_abortstackend
	msr	cpsr_c, #0x1f
	ldr	sp, =_initstackend
	bl	init
	bl	context_switch
	mov	r0, #0
	ldr	pc, =idleloop
.ltorg


.section .icode, "ax", %progbits
.align 2
idleloop:
	mcr	p15, 0, r0,c7,c0,4
	b	idleloop

.global reset
.global hang
.type reset, %function
.type hang, %function
reset:
	msr	cpsr_c, #0xd3
	mov	r0, #0x100000
	mov	r1, #0x3c800000
	str	r0, [r1]
hang:
	msr	cpsr_c, #0xd3
	mcr	p15, 0, r0,c7,c0,4
	b	hang
.size reset, .-reset
.size hang, .-hang

.type reset_handler, %function
reset_handler:
	mov	r0, #0
	adr	r1, reset_text
	b	panic
reset_text:
	.ascii	"Hit reset vector!\0"
.size reset_handler, .-reset_handler

.type undef_instr_handler, %function
undef_instr_handler:
	mov	r0, #0
	adr	r1, undef_instr_text
	sub	r2, lr, #4
	b	panicf
.size undef_instr_handler, .-undef_instr_handler

.type prefetch_abort_handler, %function
prefetch_abort_handler:
	mov	r0, #0
	adr	r1, prefetch_abort_text
	sub	r2, lr, #4
	b	panicf
.size prefetch_abort_handler, .-prefetch_abort_handler

.type data_abort_handler, %function
data_abort_handler:
	mov	r0, #0
	adr	r1, data_abort_text
	sub	r2, lr, #4
	b	panicf
.size data_abort_handler, .-data_abort_handler

.type reserved_handler, %function
reserved_handler:
	mov	r0, #0
	adr	r1, reserved_text
	b	panic
.size reserved_handler, .-reserved_handler

.type fiq_handler, %function
fiq_handler:
	mov	r0, #2
	adr	r1, fiq_text
	b	panic
.size fiq_handler, .-fiq_handler

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

syscall_text:
	.ascii	"Unhandled syscall!\0"


.section .icode.usec_timer, "ax", %progbits
.align 2
.global read_native_timer
.type read_native_timer, %function
read_native_timer:
	ldr	r0, val_3c700000
	ldr	r1, [r0,#0x80]
	ldr	r0, [r0,#0x84]
	bx	lr
.size read_native_timer, .-read_native_timer

.global read_usec_timer
.type read_usec_timer, %function
read_usec_timer:
	ldr	r0, val_3c700000
	ldr	r1, [r0,#0x80]
	ldr	r0, [r0,#0x84]
	mov	r0, r0,lsr#5
	orr	r0, r0, r1,lsl#27
	add	r0, r0, r0,asr#2
	add	r0, r0, r0,asr#6
	bx	lr
.size read_usec_timer, .-read_usec_timer

val_3c700000:
	.word	0x3c700000
