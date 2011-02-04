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


#define ASM_FILE
#include "global.h"


.section .icode.rendertext, "ax", %progbits
.align 2
.global rendertext
.type rendertext, %function
rendertext:
	stmfd	sp!, {r0-r2,r4,r5,lr}
	ldr	r4, [sp,#0x18]
	str	r4, [sp,#-4]!
	mov	r4, r3
	mov	r5, #6 * LCD_BYTESPERPIXEL
rendertext_loop:
	ldrb	r3, [r4], #1
	cmp	r3, #0
	beq	rendertext_done
	bl	renderchar
	ldmed	sp, {r0-r2}
	add	r0, r0, r5
	str	r0, [sp,#4]
	b	rendertext_loop
rendertext_done:
	ldmfd	sp!, {r0-r5,pc}
.size rendertext, .-rendertext


#if (LCD_BYTESPERPIXEL == 2)
.section .icode.renderchar, "ax", %progbits
.align 2
.global renderchar
.type renderchar, %function
renderchar:
	stmfd	sp!, {r4-r7,lr}
	ldr	r7, [sp,#0x14]
	cmn	r2, #1
	beq	renderchar_nobg
	mov	r6, r0
	mov	r4, #8
	mov	lr, r2,lsr#16
	sub	lr, lr, #0xff00
	subs	lr, lr, #0xff
	bne	renderchar_opaquerow
	mov	r12, r2,lsl#16
	mov	r12, r12,lsr#16
renderchar_blendrow:
	mov	r5, #6
renderchar_blendcol:
	ldrh	lr, [r6]
	tst	lr, #1
	orrne	lr, lr, #0x10000
	mov	lr, lr,lsr#1
	bic	lr, lr, #0x1000
	bic	lr, lr, #0x84
	add	lr, lr, r12
	strh	lr, [r6], #2
	subs	r5, r5, #1
	bne	renderchar_blendcol
	add	r6, r6, r7,lsl#1
	sub	r6, r6, #12
	subs	r4, r4, #1
	bne	renderchar_blendrow
	b	renderchar_nobg
renderchar_opaquerow:
	mov	r5, #6
renderchar_opaquecol:
	strh	r2, [r6], #2
	subs	r5, r5, #1
	bne	renderchar_opaquecol
	add	r6, r6, r7,lsl#1
	sub	r6, r6, #12
	subs	r4, r4, #1
	bne	renderchar_opaquerow
renderchar_nobg:
	adr	r5, renderchar_font
	sub	r3, r3, #0x20
	cmp	r3, #0x5f
	addcc	r5, r3,lsl#2
	addcc	r5, r3
	mov	r3, #5
renderchar_col:
	mov	r6, r0
	ldrb	r4, [r5], #1
renderchar_row:
	tst	r4, #1
	strneh	r1, [r6]
	add	r6, r6, r7,lsl#1
	movs	r4, r4,lsr#1
	bne	renderchar_row
	add	r0, r0, #2
	subs	r3, r3, #1
	bne	renderchar_col
	add	r0, r0, #2
	ldmfd	sp!, {r4-r7,pc}

renderchar_font:
	.byte 0, 0, 0, 0, 0
	.byte 0, 0, 95, 0, 0
	.byte 0, 7, 0, 7, 0
	.byte 20, 127, 20, 127, 20
	.byte 36, 42, 127, 42, 18
	.byte 35, 19, 8, 100, 98
	.byte 54, 73, 85, 34, 80
	.byte 5, 3, 0, 0, 0
	.byte 28, 34, 65, 0, 0
	.byte 0, 0, 65, 34, 28
	.byte 20, 8, 62, 8, 20
	.byte 8, 8, 62, 8, 8
	.byte 0, -96, 96, 0, 0
	.byte 8, 8, 8, 8, 8
	.byte 0, 96, 96, 0, 0
	.byte 32, 16, 8, 4, 2
	.byte 62, 81, 73, 69, 62
	.byte 0, 66, 127, 64, 0
	.byte 66, 97, 81, 73, 70
	.byte 33, 65, 69, 75, 49
	.byte 24, 20, 18, 127, 16
	.byte 39, 69, 69, 69, 57
	.byte 60, 74, 73, 73, 48
	.byte 1, 113, 9, 5, 3
	.byte 54, 73, 73, 73, 54
	.byte 6, 73, 73, 41, 30
	.byte 0, 54, 54, 0, 0
	.byte 0, 86, 54, 0, 0
	.byte 8, 20, 34, 65, 0
	.byte 20, 20, 20, 20, 20
	.byte 0, 65, 34, 20, 8
	.byte 2, 1, 81, 9, 6
	.byte 50, 73, 121, 65, 62
	.byte 124, 18, 17, 18, 124
	.byte 127, 73, 73, 73, 62
	.byte 62, 65, 65, 65, 34
	.byte 127, 65, 65, 34, 28
	.byte 127, 73, 73, 73, 65
	.byte 127, 9, 9, 9, 1
	.byte 62, 65, 73, 73, 58
	.byte 127, 8, 8, 8, 127
	.byte 0, 65, 127, 65, 0
	.byte 32, 64, 65, 63, 1
	.byte 127, 8, 20, 34, 65
	.byte 127, 64, 64, 64, 64
	.byte 127, 2, 12, 2, 127
	.byte 127, 4, 8, 16, 127
	.byte 62, 65, 65, 65, 62
	.byte 127, 9, 9, 9, 6
	.byte 62, 65, 81, 33, 94
	.byte 127, 9, 25, 41, 70
	.byte 38, 73, 73, 73, 50
	.byte 1, 1, 127, 1, 1
	.byte 63, 64, 64, 64, 63
	.byte 31, 32, 64, 32, 31
	.byte 127, 32, 24, 32, 127
	.byte 99, 20, 8, 20, 99
	.byte 3, 4, 120, 4, 3
	.byte 97, 81, 73, 69, 67
	.byte 0, 127, 65, 65, 0
	.byte 2, 4, 8, 16, 32
	.byte 0, 65, 65, 127, 0
	.byte 4, 2, 1, 2, 4
	.byte 64, 64, 64, 64, 64
	.byte 1, 2, 4, 0, 0
	.byte 32, 84, 84, 84, 120
	.byte 127, 68, 68, 68, 56
	.byte 56, 68, 68, 68, 40
	.byte 56, 68, 68, 68, 127
	.byte 56, 84, 84, 84, 24
	.byte 8, 126, 9, 1, 2
	.byte 8, 84, 84, 84, 60
	.byte 127, 4, 4, 4, 120
	.byte 0, 68, 125, 64, 0
	.byte 32, 64, 64, 61, 0
	.byte 127, 16, 40, 68, 0
	.byte 0, 65, 127, 64, 0
	.byte 124, 4, 24, 4, 120
	.byte 124, 8, 4, 4, 120
	.byte 56, 68, 68, 68, 56
	.byte 124, 20, 20, 20, 24
	.byte 8, 20, 20, 20, 124
	.byte 124, 8, 4, 4, 8
	.byte 72, 84, 84, 84, 32
	.byte 4, 63, 68, 64, 32
	.byte 60, 64, 64, 32, 124
	.byte 28, 32, 64, 32, 28
	.byte 60, 64, 56, 64, 60
	.byte 68, 40, 16, 40, 68
	.byte 12, 80, 80, 80, 60
	.byte 68, 100, 84, 76, 68
	.byte 0, 8, 54, 65, 0
	.byte 0, 0, 119, 0, 0
	.byte 0, 65, 54, 8, 0
	.byte 2, 1, 2, 4, 2
.align 2
.size renderchar, .-renderchar
#else
#error Unknown number of bytes per pixel!
#endif
