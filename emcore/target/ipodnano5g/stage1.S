@
@    iPod nano 5G (S5L8730) first-stage SRAM loader: ROM DFU hand-off state -> clocks, PLL, pads, SDRAM -> emCORE.
@
@    Runs at 0x22000000 exactly where the ROM's DFU put the IMG1 body:
@    SVC, IRQ/FIQ masked, MMU/caches off, 216/108/54 MHz from PLL0, peripheral-port remap c15,c2,4 = 0x38000012 set,
@    SP = the ROM's 0x2202bc80, USB OTG live (the DFU session that loaded us), VIC masked, SDRAM NOT initialised.
@    Initializes clocks and SDRAM using the PreEfi values cited below, then starts
@    the microsecond timer, configures GPIO pins, and builds the kernel page table.
@    PMU and LCD setup run later in the kernel.
@
@    Layout after this stage: payload (emcore.bin) copied to 0x08000000, page table at 0x2202c000, stack 0x2202bff0,
@    timer 4 counting microseconds, MMU + caches still OFF (crt0 turns them on), VIC masked.
@
@    Failure policy: every poll is bounded (WAIT_* macros, ~1 s at 216 MHz); a timeout or a failed SDRAM/copy check
@    ends in `stage1_fail` = a spin with the step code in r7 (nothing survives a reset, so there is no mailbox: a
@    device that stays silent on USB without re-enumerating as DFU 05ac:1231 died in this stage).
@
@    make N5G_NOCACHE=1: SDRAM descriptors 0x4c02 (Normal non-cacheable) instead of 0x5c06 (write-back).

.syntax unified
.cpu arm1176jzf-s
.arm

#ifdef N5G_NOCACHE
#define SDRAM_DESC 0x4c02
#else
#define SDRAM_DESC 0x5c06       /* TEX 001, C1 B1 = write-back write-allocate, AP 3  */
#endif
#define WAIT_LOOPS 0x200000     /* bounded polls */

@ r4 = 0x3c500000 (clock block), r5 = 0x3d700000 (SDRAM controller), r7 = step code for stage1_fail

@ poll until (*(base+off) & mask) == value, bounded
.macro WAIT_EQ base, off, mask, value, code
	mov	r7, #\code
	ldr	r2, =WAIT_LOOPS
9:	ldr	r0, [\base, #\off]
	ldr	r1, =\mask
	and	r0, r0, r1
	ldr	r1, =\value
	cmp	r0, r1
	beq	8f
	subs	r2, r2, #1
	bne	9b
	b	stage1_fail
8:
.endm

@ store a 32-bit literal to base+off
.macro ST base, off, value
	ldr	r0, =\value
	str	r0, [\base, #\off]
.endm

.section .text.entry, "ax"
.global _stage1_entry
_stage1_entry:
	@ ---- L0: SVC, all exceptions masked, our own stack below the page table  -------------------
	cpsid	aif, #0x13
	ldr	sp, =0x2202bff0
	@ ---- L1: CP15 as SecCore S3..S9  ------------------------------------------------------------
	mrc	p15, 0, r0, c1, c0, 0
	ldr	r1, =0x04001805		@ S3: MMU (b0), D-cache (b2), Z (b11), I-cache (b12), b26 off
	bic	r0, r0, r1
	mcr	p15, 0, r0, c1, c0, 0
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	@ S4: invalidate I + D
	mcr	p15, 0, r0, c8, c7, 0	@ S4: invalidate TLB
	mcr	p15, 0, r0, c7, c10, 4	@ S4: DSB
	mrc	p15, 0, r0, c1, c0, 1
	orr	r0, r0, #7		@ S5/S8: ACTLR RS + DB + TR (return stack, branch prediction)
	mcr	p15, 0, r0, c1, c0, 1
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x800		@ S7: Z on (A is left to crt0, which clears it and sets U)
	bic	r0, r0, #0x400000	@ S7: U off
	mcr	p15, 0, r0, c1, c0, 0
	mov	r0, #0
	mcr	p15, 0, r0, c13, c0, 0	@ S9: FCSE PID 0

	@ ---- L0b (the ROM's DFU USB core is still live and may DMA into SRAM): soft disconnect, stop
	@      the core's DMA and interrupts, core soft reset; VIC masked. emCORE's synopsysotg_init redoes the reset.
	ldr	r4, =0x38400000
	mov	r0, #2
	str	r0, [r4, #0x804]	@ DCTL = 2 soft disconnect (fw_disk:0x08039c62 does the same before the PHY reset)
	ldr	r0, [r4, #0x008]
	bic	r0, r0, #0x21
	str	r0, [r4, #0x008]	@ GAHBCFG: GlblIntrMsk + DMAEn off
	mov	r0, #0
	str	r0, [r4, #0x018]	@ GINTMSK = 0
	mov	r0, #1
	str	r0, [r4, #0x010]	@ GRSTCTL CSftRst
	WAIT_EQ r4, 0x010, 1, 0, 0x01
	ldr	r4, =0x38e00000
	mov	r0, #-1
	str	r0, [r4, #0x14]		@ VIC0INTENCLEAR
	add	r4, r4, #0x1000
	str	r0, [r4, #0x14]		@ VIC1INTENCLEAR

	@ ---- L2: clock gates (bl_PreEfi FUN_2200b17c literals 0x2200b21c..) --------------
	ldr	r4, =0x3c500000
	ST	r4, 0x48, 0x001fa7e5	@ C1 PWRCON0
	ST	r4, 0x4c, 0xfc2ae96d	@ C2 PWRCON1
	ST	r4, 0x58, 0x000001f3	@ C3 PWRCON2
	ST	r4, 0x68, 0x000380ff	@ C4 PWRCON3
	ST	r4, 0x6c, 0x06dcf779	@ C5 PWRCON4
	@ ---- L3: CLKCON0 = 0x8001, wait [3:0] == 1 (C6) --------------------------------------------------------------
	ST	r4, 0x00, 0x00008001
	WAIT_EQ r4, 0x00, 0xf, 1, 0x03
	@ ---- L4: CLKCON1 = 0, wait [21:17] == 0, wait [13:9] == 0 (C7) -----------------------------------------------
	mov	r0, #0
	str	r0, [r4, #0x04]
	WAIT_EQ r4, 0x04, 0x3e0000, 0, 0x04
	WAIT_EQ r4, 0x04, 0x3e00, 0, 0x04
	@ ---- L5: PLLMODE = 7, PLLMODE = 0, 0x3c50003c = 0 (C8, C9) ---------------------------------------------------
	mov	r0, #7
	str	r0, [r4, #0x44]
	mov	r0, #0
	str	r0, [r4, #0x44]
	str	r0, [r4, #0x3c]
	@ ---- L6: PLL0 = 24 MHz x 206 / (9 << 1) = 274.67 MHz (C10, C11, C12) -----------------------------------------
	ST	r4, 0x20, 0x0900ce01	@ C10 PLL0PMS: P 9, M 206, S 1
	ST	r4, 0x30, 0x00028488	@ C11 PLL0LCNT 165000
	mov	r0, #1
	str	r0, [r4, #0x44]		@ C12 PLLMODE = 1
	ST	r4, 0x44, 0x00010001	@ C12 PLLMODE = 0x10001
	WAIT_EQ r4, 0x40, 1, 1, 0x06	@ C12 PLLLOCK b0
	@ ---- L7: CLKCON1 = 0x404040, wait [21:17], [13:9], [5:1] == 0 (C13: HCLK = PCLK = FCLK/2) -------------------
	ST	r4, 0x04, 0x00404040
	WAIT_EQ r4, 0x04, 0x3e0000, 0, 0x07
	WAIT_EQ r4, 0x04, 0x3e00, 0, 0x07
	WAIT_EQ r4, 0x04, 0x3e, 0, 0x07
	@ ---- L8: CLKCON0 = 0x40009000, wait [3:0] == 0 (C14: FCLK = PLL0 / 1) ---------------------------------------
	ST	r4, 0x00, 0x40009000
	WAIT_EQ r4, 0x00, 0xf, 0, 0x08
	@ ---- L9: CLKCON2..5, 0x3c500070, 0x38501000 (C15, C16, C17) --------------------------------------------------
	ST	r4, 0x08, 0x80008000	@ CLKCON2
	ST	r4, 0x0c, 0xc000c000	@ CLKCON3
	ST	r4, 0x10, 0x0000c000	@ CLKCON4
	ST	r4, 0x14, 0x80008000	@ CLKCON5
	ST	r4, 0x70, 0x00008000	@ C16
	ldr	r1, =0x38501000
	mov	r0, #2
	str	r0, [r1]		@ C17 (unknown block, 4G: same)

	@ ---- T: timers (bl_PreEfi timers_init 0x2200b8bc) --------------------------------------
	@ gate window FUN_2200abb4 (opened, never restored: the kernel's timers need it)
	ldr	r0, [r4, #0x4c]
	ldr	r1, =0xe07fffdf
	and	r0, r0, r1
	str	r0, [r4, #0x4c]		@ T0 PWRCON1 &= 0xe07fffdf (timer gates b5, b23..b28)
	ldr	r0, [r4, #0x58]
	bic	r0, r0, #0x60
	str	r0, [r4, #0x58]		@ T0 PWRCON2 &= ~0x60
	ldr	r0, [r4, #0x6c]
	ldr	r1, =0xff3fff80
	and	r0, r0, r1
	str	r0, [r4, #0x6c]		@ T0 PWRCON4 &= 0xff3fff80
	ldr	r6, =0x3c700000
	mov	r0, #0x10
	str	r0, [r6, #0x004]	@ T1 timers 0..3, 5..7: CMD = 0x10
	str	r0, [r6, #0x024]
	str	r0, [r6, #0x044]
	str	r0, [r6, #0x064]
	str	r0, [r6, #0x0c4]
	str	r0, [r6, #0x0e4]
	str	r0, [r6, #0x104]
	str	r0, [r6, #0x0a4]	@ (timer 4 stopped before it is reprogrammed)
	mov	r0, #-1
	str	r0, [r6, #0x118]	@ T2 TSTAT47 = -1
	mov	r0, #0x40
	str	r0, [r6, #0x0a0]	@ T3 timer 4 CON = 0x40
	mov	r0, #0xb
	str	r0, [r6, #0x0b0]	@ T3 PRE = 0xb
	mov	r0, #-1
	str	r0, [r6, #0x0a8]	@ T3 DATA0 = -1
	mov	r0, #3
	str	r0, [r6, #0x0a4]	@ T4 CMD = 3: free-running microsecond counter at 0x3c7000b4 (udelay 0x2200cb7c)

	@ ---- G: GPIO (bl_PreEfi FUN_2200b320; tables 0x2200cc44 / 0x2200cc8c / 0x2200ccd4) ----
	ldr	r6, =0x3cf00000
	ldr	r1, =gpio_pcon
	mov	r2, #18
1:	ldr	r0, [r1], #4
	str	r0, [r6, #0x00]		@ G1 PCON(i) = T1[i]
	mov	r0, #0
	str	r0, [r6, #0x0c]		@ G2 +0x0c = T2[i] (0 except ports 0, 8)
	str	r0, [r6, #0x10]		@ G3 +0x10 = T3[i] (0 except ports 1, 10)
	add	r6, r6, #0x20
	subs	r2, r2, #1
	bne	1b
	ldr	r6, =0x3cf00000
	mov	r0, #0x20
	str	r0, [r6, #0x00c]	@ G2 port 0 +0x0c = 0x20
	mov	r0, #0x80
	str	r0, [r6, #0x10c]	@ G2 port 8 +0x0c = 0x80
	mov	r0, #0x40
	str	r0, [r6, #0x030]	@ G3 port 1 +0x10 = 0x40
	mov	r0, #0x20
	str	r0, [r6, #0x150]	@ G3 port 10 +0x10 = 0x20
	ldr	r6, =0x39700000		@ G4 GPIO interrupt controller, 7 groups: pol 0, status -1 (w1c), enable 0, type 0
	mov	r2, #7
	mov	r0, #0
	mov	r1, #-1
1:	str	r0, [r6, #0x80]
	str	r1, [r6, #0xa0]
	str	r0, [r6, #0xc0]
	str	r0, [r6, #0xe0]
	add	r6, r6, #4
	subs	r2, r2, #1
	bne	1b
	@ (board-id probe of PCON3 skipped: needs udelay + PMU)
	@ ---- L10: memory pads ------------------------------------------------
	ldr	r6, =0x3cf00380
	ST	r6, 0x0, 0x00155195	@ L10 0x3cf00380
	ST	r6, 0x4, 0x00000555	@ L10 0x3cf00384

	@ ---- L11: SDRAM (bl_PreEfi FUN_2200b4b4, literals 0x2200b5c8..0x2200b608) ------
	ST	r6, 0x4, 0x00000555	@ M0 0x3cf00384 = 0x555 again
	ldr	r5, =0x3d700000
	mov	r0, #1
	str	r0, [r5, #0x000]	@ M1
	ST	r5, 0x100, 0x00001030	@ M2 memory config
	mov	r0, #0xff
	str	r0, [r5, #0x11c]	@ M3
	str	r0, [r5, #0x120]	@ M3
	ST	r5, 0x114, 0x008aae21	@ M4 timing A
	ST	r5, 0x124, 0x050e6b46	@ M5 timing B
	mov	r0, #8
	str	r0, [r5, #0x118]	@ M6
	ST	r5, 0x108, 0x0002000b	@ M7 refresh ctrl, auto-refresh off
	mov	r0, #4
	str	r0, [r5, #0x148]	@ M8
	mov	r0, #0
	str	r0, [r5, #0x14c]	@ M8
	ST	r5, 0x140, 0x0003b3b2	@ M9 DLL: 0x3b3b2, poll +0x140 b1 (bl_PreEfi:0x2200b4f8 reads +0x140)
	WAIT_EQ r5, 0x140, 2, 2, 0x11
	ST	r5, 0x140, 0x0003b3b3	@ M9 0x3b3b3, poll (+0x144 & 3) == 3 (0x2200b504)
	WAIT_EQ r5, 0x144, 3, 3, 0x12
	ldr	r0, [r5, #0x144]
	mov	r0, r0, lsr #18
	and	r0, r0, #0xff
	mov	r0, r0, lsl #20
	ldr	r1, =0xff13b3b0
	add	r0, r0, r1
	str	r0, [r5, #0x140]	@ M10 +0x140 = ((lock >> 18 & 0xff) << 20) + 0xff13b3b0
	mov	r0, #0x10
	str	r0, [r5, #0x150]	@ M11
	@ M12 COLD command list (always cold: we never resume from hibernate). cmd = write +0x104, wait (+0x104 & 0x110000) == 0
	mov	r0, #0x33
	str	r0, [r5, #0x104]	@ M12 0x33
	bl	sdram_cmd_233		@ M12 0x233 (PALL) + wait
	bl	sdram_nop3		@ M12 0x33 x3
	bl	sdram_cmd_333		@ M12 0x333 (REF) + wait
	bl	sdram_nop3
	bl	sdram_cmd_333		@ M12 0x333 + wait
	bl	sdram_nop3
	mov	r0, #0x33
	str	r0, [r5, #0x110]	@ M12 +0x110 = 0x33 (MRS value)
	bl	sdram_cmd_133		@ M12 0x133 (MRS) + wait
	bl	sdram_nop3
	ST	r5, 0x110, 0x00008040	@ M12 +0x110 = 0x8040 (EMRS value)
	bl	sdram_cmd_133		@ M12 0x133 (EMRS) + wait
	mov	r0, #0x33
	str	r0, [r5, #0x104]	@ M12 0x33 x2
	str	r0, [r5, #0x104]
	str	r0, [r5, #0x104]	@ M13
	mov	r0, #0x40
	str	r0, [r5, #0x10c]	@ M14
	ST	r5, 0x108, 0x0002100b	@ M15 auto refresh on
	ldr	r0, [r5, #0x100]
	orr	r0, r0, #0x09000000
	orr	r0, r0, #0x00100000
	str	r0, [r5, #0x100]	@ M16 +0x100 |= 0x09100000 normal mode
	mov	r0, #0x19
	str	r0, [r5, #0x11c]	@ M17
	mov	r0, #1
	str	r0, [r5, #0x120]	@ M17
	ldr	r2, =1000
1:	ldr	r0, [r5, #0x108]	@ M18 delay: 1000 reads of +0x108 (0x2200b596: 999 + the underflow iteration)
	subs	r2, r2, #1
	bne	1b
	mov	r0, #1
	str	r0, [r5, #0x008]	@ M19 controller go
	ldr	r1, =0x3e000008
	mov	r0, #0x1f
	str	r0, [r1]		@ M20 0x3e000008 = 0x1f
	ldr	r6, =0x3ff00100
	mov	r0, #0
	str	r0, [r6]		@ M21 0x3ff00100 = 0, poll b0 == 0
	WAIT_EQ r6, 0, 1, 0, 0x21
	ldr	r1, =0x3810008c
	mov	r0, #1
	str	r0, [r1]		@ M22 0x3810008c = 1
	ldr	r1, =0x3d500400
	mov	r0, #7
	str	r0, [r1]		@ M23 0x3d500400 = 7

	@ ---- S: SDRAM sanity: 256 words at the top of the 64 MiB and 256 at the base ---------------
	mov	r7, #0x30
	ldr	r6, =0x0bf00000
	bl	sdram_test
	ldr	r6, =0x08000000
	bl	sdram_test

	@ ---- P: first-level page table at 0x2202c000 ----------------------------
	ldr	r6, =0x2202c000
	mov	r0, #0
	mov	r2, #4096
1:	str	r0, [r6], #4
	subs	r2, r2, #1
	bne	1b
	ldr	r6, =0x2202c000
	ldr	r0, =0x22004c02
	str	r0, [r6, #0x000]	@ VA 0x00000000 -> SRAM (vectors), Normal non-cacheable
	str	r0, [r6, #0x880]	@ VA 0x22000000 identity (entry 0x220)
	ldr	r0, =0x20004c02		@ VA 0x20000000..0x21ffffff -> ROM identity
	add	r1, r6, #0x800
	mov	r2, #32
1:	str	r0, [r1], #4
	add	r0, r0, #0x100000
	subs	r2, r2, #1
	bne	1b
	ldr	r0, =0x08000000 + SDRAM_DESC	@ VA 0x08000000..0x0bffffff -> SDRAM identity
	add	r1, r6, #0x200
	mov	r2, #64
1:	str	r0, [r1], #4
	add	r0, r0, #0x100000
	subs	r2, r2, #1
	bne	1b
	ldr	r0, =0x08004c02		@ VA 0x88000000..0x8bffffff -> SDRAM uncached alias
	add	r1, r6, #0x2200
	mov	r2, #64
1:	str	r0, [r1], #4
	add	r0, r0, #0x100000
	subs	r2, r2, #1
	bne	1b
	ldr	r0, =0x38004c02		@ VA 0x38000000..0x3fffffff -> peripherals identity (Apple: Normal non-cacheable)
	add	r1, r6, #0xe00
	mov	r2, #128
1:	str	r0, [r1], #4
	add	r0, r0, #0x100000
	subs	r2, r2, #1
	bne	1b

	@ ---- L12: copy the payload {u32 length, emcore.bin} appended at _stage1_end to 0x08000000, verify ------------
	ldr	r1, =_stage1_end
	ldr	r2, [r1], #4		@ length in bytes (word multiple, mkimage pads)
	ldr	r3, =0x08000000
	mov	r6, r1
	mov	r8, r2
1:	ldr	r0, [r1], #4
	str	r0, [r3], #4
	subs	r2, r2, #4
	bhi	1b
	mov	r7, #0x31
	ldr	r3, =0x08000000
1:	ldr	r0, [r6], #4
	ldr	r1, [r3], #4
	cmp	r0, r1
	bne	stage1_fail
	subs	r8, r8, #4
	bhi	1b

	@ ---- L13: jump (caches are off; drain, invalidate I, flush prefetch) ------------------------------------------
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4
	mcr	p15, 0, r0, c7, c5, 0
	mcr	p15, 0, r0, c7, c5, 4
	ldr	pc, =0x08000000

@ ---- helpers -----------------------------------------------------------------------------------------------------
sdram_cmd_233:
	ldr	r0, =0x233
	b	sdram_cmd
sdram_cmd_333:
	ldr	r0, =0x333
	b	sdram_cmd
sdram_cmd_133:
	ldr	r0, =0x133
sdram_cmd:				@ +0x104 = r0, wait (+0x104 & 0x110000) == 0
	str	r0, [r5, #0x104]
	mov	r7, #0x13
	ldr	r2, =WAIT_LOOPS
1:	ldr	r0, [r5, #0x104]
	tst	r0, #0x110000
	bxeq	lr
	subs	r2, r2, #1
	bne	1b
	b	stage1_fail
sdram_nop3:
	mov	r0, #0x33
	str	r0, [r5, #0x104]
	str	r0, [r5, #0x104]
	str	r0, [r5, #0x104]
	bx	lr

sdram_test:				@ r6 = base, r7 = fail code: 256 words of 0xa55a0000 ^ n written then read back
	mov	r2, #0
	ldr	r3, =0xa55a0000
1:	eor	r0, r3, r2
	str	r0, [r6, r2, lsl #2]
	add	r2, r2, #1
	cmp	r2, #256
	bne	1b
	mov	r2, #0
1:	eor	r0, r3, r2
	ldr	r1, [r6, r2, lsl #2]
	cmp	r0, r1
	bne	stage1_fail
	add	r2, r2, #1
	cmp	r2, #256
	bne	1b
	bx	lr

.global stage1_fail
stage1_fail:				@ r7 = step code; the device stays silent (no DFU re-enumeration)
	b	stage1_fail

.ltorg

@ GPIO table T1 (bl_PreEfi 0x2200cc44): PCON for ports 0..17 (nibble per pin: e/f output 0/1, 2/3 function)
.align 2
gpio_pcon:
	.word	0xee02eeee, 0xe0ee22ee, 0x2eeeeeee, 0x33333332, 0xee333e33, 0xeeeee2ee
	.word	0x2222222e, 0x22222222, 0x0fee33e2, 0xfee2e220, 0x2ee0e0e0, 0xeeee0222
	.word	0xeeeee332, 0xeeeeeeee, 0x220eeeee, 0xeeeeeeee, 0xee2eefee, 0xeeeeeeee
