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
	mov	r1, #0x38000000
	orr	r1, r1, #0x400000
	mov	r0, #0x20000000
	orr	r0, r0, #0x190000
	str	r0, [r1,#0x104]
	mov	pc, lr
.size lcd_init, .-lcd_init

.section .icode.lcd_shutdown, "ax", %progbits
.align 2
.global lcd_shutdown
.type lcd_shutdown, %function
lcd_shutdown:
	stmfd	sp!, {r4, lr}
	bl	displaylcd_detectlcd
	sub	r12, r12, #0x04900000
	cmp	r0, #2
	beq	lcd_shutdown_type2
	mov	r0, #0x28
	bl	displaylcd_sendlcdc
	mov	r0, #0x10
	bl	displaylcd_sendlcdc
	b	lcd_shutdown_done
lcd_shutdown_type2:
	mov	r0, #0x07
	bl	displaylcd_sendlcd2c
	mov	r0, #0x200
	orr	r0, r0, #0x32
	bl	displaylcd_sendlcd2d
	mov	r0, #0x13
	bl	displaylcd_sendlcd2c
	mov	r0, #0x1100
	orr	r0, #0x37
	bl	displaylcd_sendlcd2d
	mov	r0, #0x07
	bl	displaylcd_sendlcd2c
	mov	r0, #0x200
	orr	r0, r0, #0x01
	bl	displaylcd_sendlcd2d
	mov	r0, #0x13
	bl	displaylcd_sendlcd2c
	mov	r0, #0x100
	orr	r0, #0x37
	bl	displaylcd_sendlcd2d
	mov	r0, #0x07
	bl	displaylcd_sendlcd2c
	mov	r0, #0x200
	bl	displaylcd_sendlcd2d
	mov	r0, #0x10
	bl	displaylcd_sendlcd2c
	mov	r0, #0x680
	bl	displaylcd_sendlcd2d
	mov	r0, #0x12
	bl	displaylcd_sendlcd2c
	mov	r0, #0x160
	bl	displaylcd_sendlcd2d
	mov	r0, #0x13
	bl	displaylcd_sendlcd2c
	mov	r0, #0x100
	orr	r0, r0, #0x27
	bl	displaylcd_sendlcd2d
	mov	r0, #0x10
	bl	displaylcd_sendlcd2c
	mov	r0, #0x600
	bl	displaylcd_sendlcd2d
lcd_shutdown_done:
	ldmfd	sp!, {r4, lr}
	mov	r0, #5120
	b	sleep
.size lcd_shutdown, .-lcd_shutdown

.section .text.lcd_get_width, "ax", %progbits
.align 2
.global lcd_get_width
.type lcd_get_width, %function
lcd_get_width:
	mov	r0, #176
	mov	pc, lr
.size lcd_get_width, .-lcd_get_width

.section .text.lcd_get_height, "ax", %progbits
.align 2
.global lcd_get_height
.type lcd_get_height, %function
lcd_get_height:
	mov	r0, #132
	mov	pc, lr
.size lcd_get_height, .-lcd_get_height

.section .text.lcd_get_bytes_per_pixel, "ax", %progbits
.align 2
.global lcd_get_bytes_per_pixel
.type lcd_get_heightbytes_per_pixel, %function
lcd_get_bytes_per_pixel:
	mov	r0, #2
	mov	pc, lr
.size lcd_get_bytes_per_pixel, .-lcd_get_bytes_per_pixel

.section .icode.displaylcd, "ax", %progbits
.align 2
.global displaylcd
.type displaylcd, %function
displaylcd:
	stmfd	sp!, {r0-r4,lr}
	bl	displaylcd_sync
	bl	displaylcd_detectlcd
	sub	r12, r12, #0x04900000
	cmp	r0, #2
	bne	displaylcd_othertypes
	mov	r0, #0x50
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x51
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp,#0x04]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x52
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp,#0x08]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x53
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp,#0x0c]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x20
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x21
	bl	displaylcd_sendlcd2c
	ldr	r0, [sp,#0x08]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x22
	bl	displaylcd_sendlcd2c
	b	displaylcd_blit
displaylcd_othertypes:
	mov	r0, #0x2a
	bl	displaylcd_sendlcdc
	ldr	r0, [sp]
	bl	displaylcd_sendlcd2d
	ldr	r0, [sp,#0x04]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x2b
	bl	displaylcd_sendlcdc
	ldr	r0, [sp,#0x08]
	bl	displaylcd_sendlcd2d
	ldr	r0, [sp,#0x0c]
	bl	displaylcd_sendlcd2d
	mov	r0, #0x2c
	bl	displaylcd_sendlcdc
displaylcd_blit:
	ldmia	sp, {r0-r3}
	sub	r1, r0
	add	r1, r1, #1
	sub	r3, r2
	add	r3, r3, #1
	mul	r2, r1, r3
	ldr	r1, [sp,#0x18]
	cmp	r1, #-1
	bne	displaylcd_dma
displaylcd_pixel:
	ldr	r0, [sp,#0x1c]
	bl	displaylcd_sendlcd2d
	subs	r2, r2, #1
	bne	displaylcd_pixel
	ldmfd	sp!, {r0-r4,pc}
displaylcd_dma:
	mov	r4, #0x38000000
	orr	r4, r4, #0x400000
	mov	r0, #0x20000000
	orr	r0, r0, #0x190000
	str	r0, [r4,#0x104]
	str	r1, [r4,#0x100]
	mov	r0, r2,lsr#1
	sub	r0, r0, #1
	str	r0, [r4,#0x108]
	bl	clean_dcache
	mov	r0, #4
	str	r0, [r4,#0x114]
	ldmfd	sp!, {r0-r4,pc}

displaylcd_sendlcd2c:
	ldr	r4, [r12,#0x1c]
	tst	r4, #0x10
	bne	displaylcd_sendlcd2c
	mov	r4, r0,lsr#8
	str	r4, [r12,#0x04]
	and	r0, r0, #0xff
displaylcd_sendlcdc:
	ldrh	r4, [r12,#0x1c]
	tst	r4, #0x10
	bne	displaylcd_sendlcdc
	strh	r0, [r12,#0x04]
	mov	pc, lr
displaylcd_sendlcd2d:
	ldrh	r4, [r12,#0x1c]
	tst	r4, #0x10
	bne	displaylcd_sendlcd2d
	mov	r4, r0,lsr#8
	strh	r4, [r12,#0x40]
	and	r0, r0, #0xff
displaylcd_sendlcdd:
	ldrh	r4, [r12,#0x1c]
	tst	r4, #0x10
	bne	displaylcd_sendlcdd
	strh	r0, [r12,#0x40]
	mov	pc, lr

displaylcd_detectlcd:
	mov	r12, #0x3c000000
	orr	r12, r12, #0xf00000
	ldr	r0, [r12,#0xd0]
	bic	r0, r0, #0x0f
	str	r0, [r12,#0xd0]
	ldr	r0, [r12,#0xe0]
	bic	r0, r0, #0xf0
	str	r0, [r12,#0xe0]
	ldr	r0, [r12,#0xd4]
	and	r0, r0, #1
	ldr	r1, [r12,#0xe4]
	and	r1, r1, #2
	orr	r0, r0, r1
	mov	pc, lr
.size displaylcd, .-displaylcd


.section .icode.displaylcd_sync, "ax", %progbits
.align 2
.global displaylcd_sync
.type displaylcd_sync, %function
displaylcd_sync:
	str	lr, [sp,#-4]!
displaylcd_sync_wait:
	mov	r1, #0x38000000
	orr	r1, r1, #0x400000
	ldr	r0, [r1,#0x184]
	tst	r0, #0x70000
	ldreq	pc, [sp], #4
	b	displaylcd_sync_wait
.size displaylcd_sync, .-displaylcd_sync

.section .icode.displaylcd_busy, "ax", %progbits
.align 2
.global displaylcd_busy
.type displaylcd_busy, %function
displaylcd_busy:
	mov	r1, #0x38000000
	orr	r1, r1, #0x400000
	ldr	r0, [r1,#0x184]
	and	r0, r0, #0x70000
	mov	pc, lr
.size displaylcd_busy, .-displaylcd_busy

.section .icode.displaylcd_safe, "ax", %progbits
.align 2
.global displaylcd_safe
.type displaylcd_safe, %function
displaylcd_safe:
	mov	r1, #0x38000000
	orr	r1, r1, #0x400000
	ldr	r0, [r1,#0x184]
	tst	r0, #0x70000
	moveq	r0, #1
	movne	r0, #0
	mov	pc, lr
.size displaylcd_safe, .-displaylcd_safe

.section .icode.INT_DMA8, "ax", %progbits
.align 2
.global INT_DMA8
.type INT_DMA8, %function
INT_DMA8:
	str	lr, [sp,#-4]!
	mov	r1, #0x38000000
	orr	r1, r1, #0x400000
	mov	r0, #7
	str	r0, [r1,#0x114]
	bl	lcdconsole_callback
	ldr	pc, [sp], #4
.size INT_DMA8, .-INT_DMA8

.section .icode.lcd_translate_color, "ax", %progbits
.align 2
.global lcd_translate_color
.type lcd_translate_color, %function
lcd_translate_color:
	cmp	r0, #0xff
	moveq	r0, #-1
	moveq	pc, lr
	cmp	r0, #0
	movne	r0, #0xff000000
	orrne	r0, r0, #0xff0000
	mov	r2, r2,lsr#2
	mov	r1, r1,lsr#3
	orr	r0, r0, r3,lsr#3
	orr	r0, r0, r2,lsl#5
	orr	r0, r0, r1,lsl#11
	mov	pc, lr
.size lcd_translate_color, .-lcd_translate_color
