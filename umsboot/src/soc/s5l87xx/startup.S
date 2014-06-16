#define ASM_FILE
#include "global.h"


#ifndef SOC_S5L8701
#ifndef PAGETABLE_BASEADDR
#define PAGETABLE_BASEADDR DEFAULT_PAGETABLE_BASEADDR
#endif
#endif


.syntax unified


.extern _text_size
.extern _bss
.extern _bss_offset
.extern _dmabss
.extern _dmabss_offset
.extern _undef_instr_handler
.extern _syscall_handler
.extern _prefetch_abort_handler
.extern _data_abort_handler
.extern _reserved_handler
.extern _irq_handler
.extern _fiq_handler


.section .vectors,"ax",%progbits
    ldr pc, =_reserved_handler
    ldr pc, =_undef_instr_handler
    ldr pc, =_syscall_handler
    ldr pc, =_prefetch_abort_handler
    ldr pc, =_data_abort_handler
    ldr pc, =_reserved_handler
    ldr pc, =_irq_handler
    ldr pc, =_fiq_handler


.section .init,"ax",%progbits
    @ Check if we need to relocate ourselves
    adr r0, _init_end
    ldr r1, =_text
    cmp r0, r1
    beq _relocated
    @ Move around as necessary
    ldr r2, =(_text_size + 31)
    mov r2, r2,lsr#5
_copy:
    ldmia r0!, {r4-r11}
    stmia r1!, {r4-r11}
    subs r2, r2, #1
    bne _copy
_relocated:
    @ Check if the vectors need to be copied
    ldr r2, =_vectors_src
    sub r0, r0, r1
    add r0, r0, r2
    ldr r2, =(_vectors_size + 31)
    ldr r1, =_vectors
    mov r2, r2,lsr#5
    @ Copy vectors
_vectors_copy:
    ldmia r0!, {r4-r11}
    stmia r1!, {r4-r11}
    subs r2, r2, #1
    bne _vectors_copy
#ifdef SOC_S5L8701
    @ Detect execution base address and remap memory at 0x0 accordingly (for IRQ vectors)
    tst r1, #0x20000000
    ldr r1, =0x38200000
    ldr r0, [r1]
    orr r0, r0, #1
    bicne r0, r0, #0x10000
    orreq r0, r0, #0x10000
    str r0, [r1]
#endif

    @ Flush caches
    mov r0, #0
#if CPU_ARM_ARCH < 6
_cleancache:
#if CPU_ARM_ARCH < 5
    mcr p15, 0, r0,c7,c10,2
    add r1, r0, #0x10
    mcr p15, 0, r1,c7,c10,2
    add r1, r1, #0x10
    mcr p15, 0, r1,c7,c10,2
    add r1, r1, #0x10
    mcr p15, 0, r1,c7,c10,2
    adds r0, r0, #0x04000000
#else
    mrc p15, 0, r15,c7,c10,3
#endif
    bne _cleancache
#else
    mcr p15, 0, r0,c7,c14,0
#endif
    mcr p15, 0, r0,c7,c10,4
    mcr p15, 0, r0,c7,c5,0
#if CPU_ARM_ARCH >= 6
    mcr p15, 0, r0,c7,c5,4
#endif

#ifdef SOC_S5L8701 
    @ Enable caches
    mrc p15, 0, r1,c1,c0
    orr r1, r1, #0x00001000
    orr r1, r1, #0x00000005
    mcr p15, 0, r1,c1,c0
#else
#ifdef ENABLE_MMU
    @ Disable caches
    mrc p15, 0, r3,c1,c0
    bic r1, r3, #0x00001000
    orr r3, r3, #0x00001000
    bic r1, r1, #0x00000005
    orr r3, r3, #0x00000005
    mcr p15, 0, r1,c1,c0

    @ Flush TLB
    mcr p15, 0, r0,c8,c7

    @ Disable remapping of the first 32MB (will be done by the MMU)
    mcr p15, 0, r0,c13,c0

    @ Configure MMU
    ldr r0, =PAGETABLE_BASEADDR
    ldr r1, =0xc1e
    ldr r2, =_vectors
    add r2, r2, r1
    mcr p15, 0, r0,c2,c0
    str r2, [r0], #4
_mmuloop:
    add r1, r1, #0x00100000
    cmp r1, #0x38000000
    biccs r1, r1, #0xc
    tst r1, #0x40000000
    streq r1, [r0], #4
    beq _mmuloop
    mov r0, #-1
    mcr p15, 0, r0,c3,c0

    @ Enable caches
    mcr p15, 0, r3,c1,c0
#endif
#endif

    @ Jump to final execution address (after relocation)
    ldr pc, =_enable_irqs
.ltorg
_init_end:

.section .text,"ax",%progbits
_enable_irqs:
    @ Mask and clear all IRQs
#ifdef SOC_S5L8701 
    mov r1, #0x39c00000
    str r0, [r1,#4]
    str r0, [r1,#8]
    str r0, [r1,#0x38]
    str r0, [r1,#0x20]
    sub r0, r0, #1
    str r0, [r1]
    str r0, [r1,#0x10]
    str r0, [r1,#0x1c]
#else
    ldr r1, =0x38e00000
    add r2, r1, #0x00001000
    add r3, r1, #0x00002000
    mov r0, #-1
    str r0, [r1,#0x14]
    str r0, [r2,#0x14]
    str r0, [r1,#0xf00]
    str r0, [r2,#0xf00]
    str r0, [r3,#0x08]
    str r0, [r3,#0x0c]
#endif

    @ Set up stacks and enable IRQs
    msr cpsr_c, #0xd2
    ldr sp, =_irq_stack_top
    msr cpsr_c, #0xd7
    ldr sp, =_abort_stack_top
    msr cpsr_c, #0xdb
    ldr sp, =_abort_stack_top
    msr cpsr_c, #0x13
    ldr sp, =_stack_top

    @ Zero .bss section
    ldr r0, =_bss
    mov r1, #0
    ldr r2, =_bss_size
    bl memset

    @ Zero .dmabss section
    ldr r0, =_dmabss
    mov r1, #0
    ldr r2, =_dmabss_size
    bl memset

    @ Run C init code
    bl init
    @fallthrough

_idleloop:
    mcr p15, 0, r0,c7,c0,4
    b _idleloop
.ltorg

.global idle
.type idle, %function
idle:
    mcr p15, 0, r0,c7,c0,4
    bx lr
.size idle, .-idle


.global reset
.global hang
.type reset, %function
.type hang, %function
reset:
#ifdef SOC_S5L8701
    msr cpsr_c, #0xd3
    mov r0, #0x110000
    add r0, r0, #0xff
    add r1, r0, #0xa00
    mov r2, #0x3c800000
    str r1, [r2]
    mov r1, #0xff0
    str r1, [r2,#4]
    str r0, [r2]
#else
    msr cpsr_c, #0xd3
    mov r0, #0x100000
    mov r1, #0x3c800000
    str r0, [r1]
#endif
hang:
    msr cpsr_c, #0xd3
    mcr p15, 0, r0,c7,c0,4
    b hang
.size reset, .-reset
.size hang, .-hang

