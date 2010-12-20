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


.global irq_handler
.type irq_handler, %function
irq_handler:
	stmfd	sp!, {r0-r3,lr}
	bl	irqhandler
	ldmfd	sp!, {r0-r3,lr}
	subs	pc, lr, #4

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
	bic	r0, r0, #5
	mcr	p15, 0, r0,c1,c0
	mcr	p15, 0, r0,c7,c5
	bx	r1
.size execfirmware, .-execfirmware
