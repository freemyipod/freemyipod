@
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
@    You should have received a copy of the GNU General Public License
@    along with emCORE.  If not, see <http://www.gnu.org/licenses/>.
@
@

#define ASM_FILE
#include "global.h"

.section .icode.contextswitch, "ax", %progbits
.align 2
.global yield
.type yield, %function
yield:
	mrs	r1, cpsr
	msr	cpsr_c, #0xdf
	ldr	r0, =current_thread
	ldr	r0, [r0]
	stmia	r0, {r0-r14}
	str	lr, [r0,#0x3c]
	str	r1, [r0,#0x40]
	msr	cpsr_c, #0xd2
	mov	r4, r0
	bl	scheduler_pause_accounting
switch_thread:
	adr	lr, resume_thread
	mov	r0, #0
	mov	r1, r4
	b	scheduler_switch
.size yield, .-yield

.global leave_thread
.type leave_thread, %function
leave_thread:
	msr	cpsr_c, #0xd2
	mov	r4, #0
	b	switch_thread
.size leave_thread, .-leave_thread

.global syscall_handler
.type syscall_handler, %function
syscall_handler:
	ldr	sp, [lr,#-4]
	bics	sp, sp, #0xff000000
	beq	syscall_breakpoint
	cmp	sp, #(swilist_end-swilist_start)/4+1
	bcs	unknown_swi
	mov	r12, lr
	mrs	sp, spsr
	msr	cpsr_c, sp
	mov	lr, r12
	ldr	r12, [lr,#-4]
	bic	r12, r12, #0xff000000
	sub	r12, r12, #2
	add	pc, pc, r12,lsl#2
swilist_start:
	b	panicf
	b	get_syscall_table
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
	bl	scheduler_pause_accounting
	adr	lr, resume_thread
	mov	r0, #0
	mov	r1, #0
	b	scheduler_switch
	
unknown_swi:
	ldr	sp, =_abortstackend - 0x44
	str	lr, [sp,#-4]
	mrs	lr, spsr
	str	lr, [sp,#-8]
	msr	cpsr_c, #0xdb
	ldr	lr, [sp,#-8]
	msr	spsr_all, lr
	ldr	lr, [sp,#-4]
	b	undef_instr_handler
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
	bl	scheduler_pause_accounting
	bl	irqhandler
@ fallthrough

.global resume_thread
.type resume_thread, %function
resume_thread:
	bl	scheduler_resume_accounting
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
	ldr	sp, =_abortstackend
	stmfd	sp!, {r0-r2}
	bl	interrupt_shutdown
	mov	r0, sp
	msr	cpsr_c, #0xd3
	mov	sp, r0
	ldmfd	sp, {r0-r2}
	bl	memmove
	bl	clean_dcache
#ifdef HAVE_TARGETINIT_EXECFIRMWARE
	bl	targetinit_execfirmware
#endif
	ldr	r1, [sp]
	mrc	p15, 0, r0,c1,c0
	bic	r0, r0, #5
	mcr	p15, 0, r0,c1,c0
	mov	r0, #0
	mcr	p15, 0, r0,c7,c5
	bx	r1
.size execfirmware, .-execfirmware

.global panic_recover
.type panic_recover, %function
panic_recover:
	and	r0, r0, #0x1f
	cmp	r0, #0x12
	beq	irqpanic
	cmp	r0, #0x17
	cmpne	r0, #0x1b
	bne	yield
	ldr	sp, =_abortstackend - 0x44
	ldr	r0, [sp,#0x40]
	and	r0, r0, #0x1f
	cmp	r0, #0x12
	beq	irqabort
	ldr	r9, =current_thread
	ldr	lr, [r9]
	ldmia	sp!, {r0-r7}
	stmia	lr!, {r0-r7}
	ldmia	sp!, {r0-r8}
	stmia	lr!, {r0-r8}
	msr	cpsr_c, #0xd2
	bl	scheduler_pause_accounting
	adr	lr, resume_thread
	mov	r0, #0
	mov	r1, r9
	b	scheduler_switch
.size panic_recover, .-panic_recover

irqabort:
	mov	r0, #CONSOLE_PANIC
	adr	r1, irqabort_string
	ldmfd	sp!, {r2,r3}
	bl	csprintf
	mov	r0, #CONSOLE_PANIC
	bl	csflush
	mov	r0, #CONSOLE_PANICDUMP
	adr	r1, stack_string
	bl	csputs
	add	sp, sp, #0x3c
	ldr	r4, [sp,#-0x10]
	sub	r4, r4, #0x800
	add	r5, r4, #0x2000
irqabort_stackline:
	mov	r0, #CONSOLE_PANICDUMP
	adr	r1, hexdump_string
	mov	r2, r4
	ldr	r3, [r4], #4
	bl	csprintf
	cmp	r4, r5
	bne	irqabort_stackline
	mov	r0, #CONSOLE_PANICDUMP
	bl	csflush
	b	hang

irqpanic:
	mov	r0, #CONSOLE_PANIC
	adr	r1, irqpanic_string
	bl	csputs
	mov	r0, #CONSOLE_PANIC
	bl	csflush
@ fallthrough

.global panic_fatal
.type panic_fatal, %function
panic_fatal:
	b	hang
.size panic_fatal, .-panic_fatal

stack_string:
	.ascii "Stack:\n\0"
	
hexdump_string:
	.ascii "%08X: %08X\n\0"

irqabort_string:
	.ascii "Exception in IRQ handler!\n"
	.ascii " R0: %08X  R1: %08X\n R2: %08X  R3: %08X\n"
	.ascii " R4: %08X  R5: %08X\n R6: %08X  R7: %08X\n"
	.ascii " R8: %08X  R9: %08X\nR10: %08X R11: %08X\n"
	.ascii "R12: %08X R13: %08X\nR14: %08X R15: %08X\n"
	.ascii "CPSR: %08X\n\0"
	
irqpanic_string:
	.ascii "Panic in IRQ handler!\n\0"
		
