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
	ldr	r0, =current_thread
	ldr	r0, [r0]
	stmia	r0!, {r0-r14}
	str	lr, [r0], #4
	mrs	r1, cpsr
	str	r1, [r0]
	adr	lr, resume_thread
	b	scheduler_switch
.size context_switch, .-context_switch

.global irq_handler
.type irq_handler, %function
irq_handler:
	str	r12, [sp,#-4]
	ldr	r12, =current_thread
	ldr	r12, [r12]
	stmia	r12!, {r0-r11}
	ldr	r0, [sp,#-4]
	mrs	r5, cpsr
	mrs	r4, spsr
	orr	r6, r4, #0xc0
	msr	cpsr_c, r6
	mov	r1, sp
	mov	r2, lr
	msr	cpsr_c, r5
	sub	r3, lr, #4
	stmia	r12, {r0-r4}
	bl	irqhandler
@ fallthrough

.global resume_thread
.type resume_thread, %function
resume_thread:
	ldr	r0, =current_thread
	ldr	r0, [r0]
	ldr	r1, [r0,#0x40]
	orr	r2, r1, #0xc0
	msr	cpsr_c, r2
	ldr	sp, [r0,#0x34]
	ldr	lr, [r0,#0x38]
	msr	cpsr_c, #0xd2
	msr	spsr_all, r1
	ldr	lr, [r0,#0x3c]
	ldmia	r0, {r0-r12}
	movs	pc, lr
.size irq_handler, .-irq_handler
.size resume_thread, .-resume_thread

.global enter_critical_section
.type enter_critical_section, %function
enter_critical_section:
	mrs	r0, cpsr
	msr	cpsr_c, #0xd3
	mov	pc, lr
.size enter_critical_section, .-enter_critical_section

.global leave_critical_section
.type leave_critical_section, %function
leave_critical_section:
	msr	cpsr_c, r0
	mov	pc, lr
.size leave_critical_section, .-leave_critical_section
