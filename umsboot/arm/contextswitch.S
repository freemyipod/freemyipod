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


.section .icode.contextswitch, "ax", %progbits
.align 2
.global context_switch
.type context_switch, %function
context_switch:
	mrs	r1, cpsr
	msr	cpsr_c, #0xdf
	ldr	r0, =current_thread
	ldr	r0, [r0]
	stmia	r0!, {r0-r14}
	str	lr, [r0], #4
	str	r1, [r0]
	msr	cpsr_c, #0xd2
	adr	lr, resume_thread
	mov	r0, #-1
	b	scheduler_switch
.size context_switch, .-context_switch

.global syscall_handler
.type syscall_handler, %function
syscall_handler:
	ldr	sp, [lr,#-4]
	bics	sp, sp, #0xff000000
	beq	syscall_breakpoint
	mov	r12, lr
	mrs	sp, spsr
	msr	cpsr_c, sp
	mov	lr, r12
	ldr	r12, [lr,#-4]
	bic	r12, r12, #0xff000000
	cmp	r12, #(swilist_end-swilist_start)/4+1
	movcs	r0, #0
	addcc	pc, pc, r12,lsl#2
	adr	r1, unknown_swi_string
	mov	r2, r12
swilist_start:
	b	panicf
	b	get_syscall_table
	b	__clzsi2
	b	__aeabi_uidivmod
	b	__aeabi_idivmod
swilist_end:

syscall_breakpoint:
	msr	cpsr_c, #0xd3
	ldr	sp, =current_thread
	ldr	sp, [sp]
	stmia	sp!, {r0-r12}
	mov	r2, lr
	mrs	r3, spsr
	mov	r4, #1
	mov	r5, sp
	msr	cpsr_c, #0xdf
	mov	r0, sp
	mov	r1, lr
	msr	cpsr_c, #0xd2
	stmia	r5, {r0-r4}
	adr	lr, resume_thread
	mov	r0, #-1
	b	scheduler_switch
.size syscall_handler, .-syscall_handler

.global irq_handler
.type irq_handler, %function
was_in_svc_mode:
	msr	cpsr_c, #0xd3
	sub	r3, lr, #4
	mrs	r4, spsr
	msr	cpsr_c, r5
	b enter_irqhandler

irq_handler:
	str	r12, [sp,#-4]
	ldr	r12, =current_thread
	ldr	r12, [r12]
	stmia	r12!, {r0-r11}
	ldr	r0, [sp,#-4]
	mrs	r5, cpsr
	mrs	r4, spsr
	msr	cpsr_c, #0xdf
	mov	r1, sp
	mov	r2, lr
	and	r3, r4, #0x1f
	cmp	r3, #0x13
	beq	was_in_svc_mode
	msr	cpsr_c, r5
	sub	r3, lr, #4
enter_irqhandler:
	stmia	r12, {r0-r4}
	bl	irqhandler
@ fallthrough

.global resume_thread
.type resume_thread, %function
resume_thread:
	ldr	lr, =current_thread
	ldr	lr, [lr]
	mov	r0, lr
	ldr	r1, [lr,#0x40]
	ldr	lr, [lr,#0x3c]
	msr	spsr_all, r1
	msr	cpsr_c, 0xdf
	ldmia	r0, {r0-r14}
	msr	cpsr_c, #0xd2
	movs	pc, lr
.size irq_handler, .-irq_handler
.size resume_thread, .-resume_thread

.global enter_critical_section
.type enter_critical_section, %function
enter_critical_section:
	mrs	r0, cpsr
	orr	r1, r0, #0xc0
	msr	cpsr_c, r1
	mov	pc, lr
.size enter_critical_section, .-enter_critical_section

.global leave_critical_section
.type leave_critical_section, %function
leave_critical_section:
	msr	cpsr_c, r0
	mov	pc, lr
.size leave_critical_section, .-leave_critical_section

.global execfirmware
.type execfirmware, %function
execfirmware:
	str	r0, [sp,#-4]!
	bl	interrupt_shutdown
	bl	clean_dcache
	ldr	r1, [sp], #4
	msr	cpsr_c, #0xd3
	mrc	p15, 0, r0,c1,c0
	bic	r0, r0, #1
	mcr	p15, 0, r0,c1,c0
	mcr	p15, 0, r0,c7,c5
	bx	r1
.size execfirmware, .-execfirmware

unknown_swi_string:
	.ascii "Unhandled SWI %06X\0"
