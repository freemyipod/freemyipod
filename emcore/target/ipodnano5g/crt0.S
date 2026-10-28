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

@ iPod nano 5G (S5L8730) kernel entry. Adapted from the nano 4G kernel entry:
@  - entered at 0x08000000 by target/ipodnano5g/stage1.S with: SVC, IRQ/FIQ/abort masked, MMU + caches OFF, the
@    first-level page table built at 0x2202c000, clocks/SDRAM/timer 4 running;
@  - the 4G's dead page-table writes are gone: TTBR0/DACR are written here and the MMU +
@    caches are enabled (N5G_NOCACHE=1 keeps the caches off, see target.mk);
@  - reset: WDTCON 0x3c800000 = 0 then SWRCON 0x3c500050 = 0xa5 (fw_disk system_reset 0x08070c04;
@    the 4G wrote 0x100000 to the watchdog);
@  - read_usec_timer: timer 4 counter 0x3c7000b4 started by stage 1 (bl_PreEfi:0x2200b8bc).

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
	cpsid	aif
	mrc	p15, 0, r0,c1,c0
	bic	r0, r0, #0x2000		@ V = 0: exception vectors at VA 0 (= SRAM 0x22000000 through the page table)
	bic	r0, r0, #0x200		@ R = 0
	orr	r0, r0, #0x100		@ S = 1 (as the 4G)
	mcr	p15, 0, r0,c1,c0
	mov	r0, #0
	mcr	p15, 0, r0,c2,c0,2	@ TTBCR = 0: TTBR0 only
	ldr	r0, =0x2202c000		@ stage-1 page table (stage1.S)
	mcr	p15, 0, r0,c2,c0,0	@ TTBR0
	mov	r0, #0x7fffffff		@ DACR: 0x7fffffff = domains 0..14 manager, 15 client (as the 4G; Apple writes DACR = 3; all domain-0 sections are AP 3 anyway)
	mcr	p15, 0, r0,c3,c0
	mov	r0, #0
	mcr	p15, 0, r0,c8,c7,0	@ invalidate unified TLB
	mcr	p15, 0, r0,c7,c7,0	@ invalidate I + D caches (they were off: nothing dirty)
	mcr	p15, 0, r0,c7,c10,4	@ DSB
	mrc	p15, 0, r0,c1,c0
	orr	r0, r0, #1		@ M: MMU on
#ifndef N5G_NOCACHE
	orr	r0, r0, #4		@ C: D-cache on (SDRAM is write-back 0x5c06)
	orr	r0, r0, #0x1000		@ I: I-cache on
#endif
	orr	r0, r0, #0x400000	@ U: unaligned access support (ARMv6, as the 4G)
	mcr	p15, 0, r0,c1,c0
	mcr	p15, 0, r0,c7,c5,4	@ flush prefetch buffer
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
	ldr	r0, =_ibssstart
	ldr	r1, =_ibssend
	mov	r2, #0
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
	mov	r0, #0
	mcr	p15, 0, r0,c7,c10,0 @ clean data cache
	mcr	p15, 0, r0,c7,c10,4 @ drain write buffer
	mcr	p15, 0, r0,c7,c5,0  @ invalidate instruction cache
	mcr	p15, 0, r0,c7,c5,6  @ flush branch target cache
	mcr	p15, 0, r0,c7,c5,4  @ flush prefetch buffer
	@ VIC0/VIC1 (PL192): mask everything, acknowledge, clear the per-IRQ words 0x38e02008/0c
	ldr	r1, =0x38e00000
	add	r2, r1, #0x00001000
	add	r3, r1, #0x00002000
	mov	r4, #-1
	str	r4, [r1,#0x14]
	str	r4, [r2,#0x14]
	str	r4, [r1,#0xf00]
	str	r4, [r2,#0xf00]
	str	r4, [r3,#0x08]
	str	r4, [r3,#0x0c]
	msr	cpsr_c, #0xd2
	ldr	sp, =_irqstackend
	msr	cpsr_c, #0xd7
	ldr	sp, =_abortstackend
	msr	cpsr_c, #0xdb
	ldr	sp, =_abortstackend
	msr	cpsr_c, #0x1f
	ldr	sp, =_abortstackend
	bl	init
	bl	yield
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
	mov	r0, #0
	mov	r1, #0x3c800000		@ WDTCON = 0 (fw_disk:0x08070c04)
	str	r0, [r1]
	mov	r0, #0xa5
	ldr	r1, =0x3c500050		@ SWRCON = 0xa5: software reset (4G used 0xaaa5)
	str	r0, [r1]
hang:
	msr	cpsr_c, #0xd3
	mov	r0, #0
	mcr	p15, 0, r0,c7,c0,4
	b	hang
.size reset, .-reset
.size hang, .-hang

.type reset_handler, %function
reset_handler:
	stmfd	sp, {r10-r12}
	mov	r10, sp
	mov	r11, lr
	mrs	r12, cpsr
	msr	cpsr_c, #0xd7
	sub	sp, sp, #0x44
	stmia	sp!, {r0-r9}
	sub	r0, r10, #0xc
	ldmia	r0, {r0-r2}
	mov	r3, r10
	mov	r4, r11
	mov	r5, r11
	mov	r6, r12
	stmia	sp!, {r0-r6}
	sub	sp, sp, #0x44
	mov	r0, #0
	adr	r1, reset_text
	mov	r2, r11
	b	panicf
.size reset_handler, .-reset_handler

.global undef_instr_handler
.type undef_instr_handler, %function
undef_instr_handler:
	sub	sp, sp, #0x44
	stmia	sp!, {r0-r12}
	sub	r2, lr, #4
	mrs	r3, spsr
	mrs	r4, cpsr
	orr	r0, r3, #0xc0
	msr	cpsr_c, r0
	mov	r0, sp
	mov	r1, lr
	msr	cpsr_c, r4
	stmia	sp!, {r0-r3}
	sub	sp, sp, #0x44
	mov	r0, #0
	adr	r1, undef_instr_text
	ldr	r3, [r2]
	b	panicf
.size undef_instr_handler, .-undef_instr_handler

.type prefetch_abort_handler, %function
prefetch_abort_handler:
	sub	sp, sp, #0x44
	stmia	sp!, {r0-r12}
	sub	r2, lr, #4
	mrs	r3, spsr
	mrs	r4, cpsr
	orr	r0, r3, #0xc0
	msr	cpsr_c, r0
	mov	r0, sp
	mov	r1, lr
	msr	cpsr_c, r4
	stmia	sp!, {r0-r3}
	sub	sp, sp, #0x44
	mov	r0, #0
	adr	r1, prefetch_abort_text
	mrc	p15, 0, r3,c5,c0
	mov	r4, r3,lsr#4
	and	r4, r4, #0xf
	and	r5, r3, #0xf
	stmfd	sp!, {r4-r5}
	b	panicf
.size prefetch_abort_handler, .-prefetch_abort_handler

.type data_abort_handler, %function
data_abort_handler:
	sub	sp, sp, #0x44
	stmia	sp!, {r0-r12}
	sub	r2, lr, #8
	mrs	r3, spsr
	mrs	r4, cpsr
	orr	r0, r3, #0xc0
	msr	cpsr_c, r0
	mov	r0, sp
	mov	r1, lr
	msr	cpsr_c, r4
	stmia	sp!, {r0-r3}
	sub	sp, sp, #0x44
	mov	r0, #0
	adr	r1, data_abort_text
	mrc	p15, 0, r3,c5,c0
	mov	r4, r3,lsr#4
	and	r4, r4, #0xf
	and	r5, r3, #0xf
	mrc	p15, 0, r6,c6,c0
	stmfd	sp!, {r4-r6}
	b	panicf
.size data_abort_handler, .-data_abort_handler

.type reserved_handler, %function
reserved_handler:
	stmfd	sp, {r10-r12}
	mov	r10, sp
	mov	r11, lr
	mrs	r12, cpsr
	msr	cpsr_c, #0xd7
	sub	sp, sp, #0x44
	stmia	sp!, {r0-r9}
	sub	r0, r10, #0xc
	ldmia	r0, {r0-r2}
	mov	r3, r10
	mov	r4, r11
	mov	r5, r11
	mov	r6, r12
	stmia	sp!, {r0-r6}
	sub	sp, sp, #0x44
	mov	r0, #0
	adr	r1, reserved_text
	mov	r2, r11
	b	panicf
.size reserved_handler, .-reserved_handler

.type fiq_handler, %function
fiq_handler:
	mov	r0, #2
	adr	r1, fiq_text
	b	panic
.size fiq_handler, .-fiq_handler

prefetch_abort_text:
	.ascii	"Prefetch abort at %08X!\nFSR: %08X (domain %d, fault %d)\0"

reset_text:
	.ascii	"Hit reset vector!\n(Last known PC: %08X)\0"

undef_instr_text:
	.ascii	"Undefined instruction at %08X!\n(Opcode: %08X)\0"

data_abort_text:
	.ascii	"Data abort at %08X!\nFSR: %08X (domain %d, fault %d)\nAddress: %08X\0"

fiq_text:
	.ascii	"Unhandled FIQ!\0"

reserved_text:
	.ascii	"Hit reserved exception handler!\0"

syscall_text:
	.ascii	"Unhandled syscall!\0"


.section .icode.usec_timer, "ax", %progbits
.align 2
.global read_native_timer
.type read_native_timer, %function
read_native_timer:
	@ 64-bit system counter at 0x3c700080 (+0x84 count, bootrom:0x200033b8): low word in r0,
	@ high word in r1 as the 4G's read_native_timer returned them
	ldr	r0, val_3c700000
	ldr	r1, [r0,#0x80]
	ldr	r0, [r0,#0x84]
	bx	lr
.size read_native_timer, .-read_native_timer

.global read_usec_timer
.type read_usec_timer, %function
read_usec_timer:
	@ timer 4 counter 0x3c7000b4 = microseconds (bl_PreEfi udelay 0x2200cb7c; started by stage1.S step T1..T4)
	ldr	r0, val_3c700000
	ldr	r0, [r0,#0xb4]
	bx	lr
.size read_usec_timer, .-read_usec_timer

val_3c700000:
	.word	0x3c700000
