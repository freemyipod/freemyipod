#include "global.h"
#include "sys/util.h"

void enter_critical_section()
{
#ifdef CPU_ARM_CORTEX
    __asm__ volatile("cpsid if");
#else
    __asm__ volatile(
        "mrs r0, cpsr       \n\t"
        "orr r0, r0, #0xc0  \n\t"
        "msr cpsr_c, r0     \n\t"
    ::: "r0");
#endif
}

void leave_critical_section()
{
#ifdef CPU_ARM_CORTEX
    __asm__ volatile("cpsie if");
#else
    __asm__ volatile(
        "mrs r0, cpsr       \n\t"
        "bic r0, r0, #0xc0  \n\t"
        "msr cpsr_c, r0     \n\t"
    ::: "r0");
#endif
}

void clean_dcache(const void* addr, uint32_t len)
{
#ifdef CPU_ARM_OLD
    __asm__ volatile(
        "mov r0, #0               \n\t"
#if defined(CPU_ARM_OLD_V6)
        "mcr p15, 0, r0,c7,c10,0  \n\t"
#elif defined(CPU_ARM_OLD_V5)
        "0:                       \n\t"
        "mrc p15, 0, r15,c7,c10,3 \n\t"
        "bne 0b                   \n\t"
#elif defined(CPU_ARM_OLD_V4)
        "0:                       \n\t"
        "mcr p15, 0, r0,c7,c10,2  \n\t"
        "add r1, r0, #0x10        \n\t"
        "mcr p15, 0, r1,c7,c10,2  \n\t"
        "add r1, r1, #0x10        \n\t"
        "mcr p15, 0, r1,c7,c10,2  \n\t"
        "add r1, r1, #0x10        \n\t"
        "mcr p15, 0, r1,c7,c10,2  \n\t"
        "adds r0, r0, #0x04000000 \n\t"
        "bne 0b                   \n\t"
#else
#error Please implement invalidate_dcache for this ARM core generation!
#endif
        "mcr p15, 0, r0,c7,c10,4 \n\t"
    ::: "r0", "r1");
#endif
}

void invalidate_dcache(const void* addr, uint32_t len)
{
#ifdef CPU_ARM_OLD
    __asm__ volatile(
        "mov r0, #0               \n\t"
#if defined(CPU_ARM_OLD_V6)
        "mcr p15, 0, r0,c7,c14,0  \n\t"
#elif defined(CPU_ARM_OLD_V5)
        "0:                       \n\t"
        "mrc p15, 0, r15,c7,c14,3 \n\t"
        "bne 0b                   \n\t"
#elif defined(CPU_ARM_OLD_V4)
        "0:                       \n\t"
        "mcr p15, 0, r0,c7,c14,2  \n\t"
        "add r1, r0, #0x10        \n\t"
        "mcr p15, 0, r1,c7,c14,2  \n\t"
        "add r1, r1, #0x10        \n\t"
        "mcr p15, 0, r1,c7,c14,2  \n\t"
        "add r1, r1, #0x10        \n\t"
        "mcr p15, 0, r1,c7,c14,2  \n\t"
        "adds r0, r0, #0x04000000 \n\t"
        "bne 0b                   \n\t"
#else
#error Please implement invalidate_dcache for this ARM core generation!
#endif
        "mcr p15, 0, r0,c7,c10,4 \n\t"
    ::: "r0", "r1");
#endif
}

void invalidate_icache(const void* addr, uint32_t len)
{
#ifdef CPU_ARM_OLD
    __asm__ volatile(
        "mov r0, #0              \n\t"
        "mcr p15, 0, r0,c7,c5,0  \n\t"
#ifdef CPU_ARM_OLD_V6
        "mcr p15, 0, r0,c7,c5,4  \n\t"
#endif
    ::: "r0");
#endif
}

void enable_mmu()
{
#ifdef CPU_ARM_OLD
    __asm__ volatile(
        "mrc p15, 0, r0,c1,c0    \n\t"
        "orr r1, r1, #0x00001000 \n\t"
        "orr r1, r1, #0x00000005 \n\t"
        "mcr p15, 0, r0,c1,c0    \n\t"
    ::: "r0");
#endif
}

void disable_mmu()
{
#ifdef CPU_ARM_OLD
    __asm__ volatile(
        "mrc p15, 0, r0,c1,c0    \n\t"
        "bic r1, r1, #0x00001000 \n\t"
        "bic r1, r1, #0x00000005 \n\t"
        "mcr p15, 0, r0,c1,c0    \n\t"
    ::: "r0");
#endif
}

__attribute__((naked, noinline)) void* memset(void* dst_void, int val, size_t len)
{
    __asm__ volatile(
        ".syntax unified             \n"
        "    mov r3, r0              \n"
#ifndef SQUEEZE
        "    cmp r2, #8              \n"
        "    blt 2f                  \n"
        "    orr r1, r1, r1, lsl #8  \n"
        "    orr r1, r1, r1, lsl #16 \n"
        "1:                          \n"
        "    tst r3, #3              \n"
        "    itt ne                  \n"
        "    strbne r1, [r3], #1     \n"
        "    subne r2, r2, #1        \n"
        "    bne 1b                  \n"
        "    mov r12, r1             \n"
        "1:                          \n"
        "    cmp r2, #8              \n"
        "    blt 2f                  \n"
        "    stmia r3!, {r1, r12}    \n"
        "    sub r2, r2, #8          \n"
        "    cmp r2, #8              \n"
        "    blt 2f                  \n"
        "    stmia r3!, {r1, r12}    \n"
        "    sub r2, r2, #8          \n"
        "    cmp r2, #8              \n"
        "    blt 2f                  \n"
        "    stmia r3!, {r1, r12}    \n"
        "    sub r2, r2, #8          \n"
        "    cmp r2, #8              \n"
        "    itt ge                  \n"
        "    stmiage r3!, {r1, r12}  \n"
        "    subge r2, r2, #8        \n"
        "    bge 1b                  \n"
        "2:                          \n"
#endif
        "    movs r2, r2             \n"
        "    it eq                   \n"
        "    bxeq lr                 \n"
        "1:                          \n"
        "    strb r1, [r3], #1       \n"
        "    subs r2, r2, #1         \n"
        "    bne 1b                  \n"
        "    bx lr                   \n"
    );
}

__attribute__((naked, noinline)) void* memmove(void* dst, const void* src, size_t len)
{
    __asm__ volatile(
        ".syntax unified                     \n"
        // Copy memory forward or backward, allows overlapping src/dest
        // args: R0: dest, R1: src, R2: len; returns: dest
        "    cmp r1, r0                      \n"  // TEST src - dst
        // If src == dst we don't need to do anything at all
        "    it eq                           \n"  // IF src == dest:
        "    bxeq lr                         \n"  //     THEN return dest
        "    stmfd sp!, {r0, lr}             \n"  // SAVE return_addr orig_dest                         // STACK: orig_dest return_addr
        // Check if we need to go backwards (if source < destination and there is overlap)
        "    itt cc                          \n"  // IF src < dest:
        "    addcc r3, r1, r2                \n"  //     THEN src_end = src + len
        "    cmpcc r0, r3                    \n"  //     THEN TEST dest - src_end
        "    bcc 9f                          \n"  // IF (src < dest) AND (dest < src_end): THEN GOTO [backward]
        "                                    \n"  // (fallthrough)

#if defined(SQUEEZE)
        // Absolute minimum size forward implementation:
        "1:                                  \n"  // DO:
        // Stupidly copy a byte at a time
        "    subs r2, r2, #1                 \n"  //     len--
        "    it lt                           \n"  //     IF len < 0:
        "    ldmfdlt sp!, {r0, pc}           \n"  //         THEN RETURN orig_dest
        "    ldrb r3, [r1], #1               \n"  //     R3 = *src++ (byte)
        "    strb r3, [r0], #1               \n"  //     *dest++ = R3 (byte)
        "    b 1b                            \n"  // WHILE true
        // End of absolute minimum size forward implementation

#elif defined(OPTIMIZE_SIZE)
        // Size optimized forward implementation:
        // Skip to tail if we have to copy less than 4 bytes
        "    subs r2, r2, #4                 \n"  // len -= 4                                           // len offset: -4
        "    blt 2f                          \n"  // IF len < 0: THEN GOTO [remainder]
        // Skip to tail if src or dest are misaligned
        "    tst r0, #3                      \n"  // TEST dest & 3
        "    it eq                           \n"  // IF !(dest & 3):
        "    tsteq r1, #3                    \n"  //     THEN TEST src & 3
        "    bne 2f                          \n"  // IF (dest & 3) OR (src & 3): THEN GOTO [remainder]
        // Copy 4 bytes at a time until less than 4 are remaining
        "1:                                  \n"  // DO:
        "    ldr r3, [r1], #4                \n"  //     R3 = *src++ (word)
        "    str r3, [r0], #4                \n"  //     *dest++ = R3 (word)
        "    subs r2, r2, #4                 \n"  //     len -= 4
        "    bge 1b                          \n"  // WHILE: len >= 0
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "2:                                  \n"  // [remainder]: copy the remainder byte by byte
        // Correct length offset from 4 byte copying mode
        "    adds r2, r2, #4                 \n"  // len += 4                                           // len offset: 0
        "1:                                  \n"  // DO:
        // Copy the remainder a byte at a time
        "    it eq                           \n"  //     IF len == 0:
        "    ldmfdeq sp!, {r0, pc}           \n"  //         THEN RETURN orig_dest
        "    ldrb r3, [r1], #1               \n"  //     R3 = *src++ (byte)
        "    strb r3, [r0], #1               \n"  //     *dest++ = R3 (byte)
        "    subs r2, r2, #1                 \n"  //     len--
        "    b 1b                            \n"  // WHILE true
        // End of size optimized forward implementation
#else

        // Performance optimized forward implementation:
        // Skip to 3 byte tail if we have to copy less than 4 bytes
        "    subs r2, r2, #4                 \n"  // len -= 4                                           // len offset: -4
        "    blt 6f                          \n"  // IF len < 0: THEN GOTO [remainder3]
        // If the destination is misaligned, align it
        // This will return to label 3 if the source is aligned after the destination was aligned
        "    ands r12, r0, #3                \n"  // misalignment = dest & 3
        "    bne 7f                          \n"  // IF misalignment != 0: THEN GOTO [align]
        // The destination was already aligned, but the source isn't
        // We have no option but to fall back to byte by byte copying
        "    tst r1, #3                      \n"  // TEST src & 3
        "    bne 8f                          \n"  // IF src & 3: THEN GOTO [bytewise]
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "3:                                  \n"  // [aligned]: we are at least 4 byte aligned
        // Skip to 11 byte tail if we have to copy less than 12 bytes
        "    subs r2, r2, #8                 \n"  // len -= 8                                           // len offset: -12
        "    blt 5f                          \n"  // IF len < 0: THEN GOTO [remainder11]
        // Skip to 31 byte tail if we have to copy less than 32 bytes
        "    subs r2, r2, #0x14              \n"  // len -= 20                                          // len offset: -32
        "    blt 4f                          \n"  // IF len < 0: THEN GOTO [remainder31]
        // Save R4 so that we have an additional copying scratchpad register
        "    str r4, [sp, #-4]!              \n"  // SAVE R4                                            // STACK: R4 orig_dest return_addr
        "1:                                  \n"  // DO:
        // Copy 32 bytes at a time
        "    ldmia r1!, {r3, r4, r12, lr}    \n"  //     {R3,R4,R12,LR} = *src++ (qword)
        "    stmia r0!, {r3, r4, r12, lr}    \n"  //     *dest++ = {R3,R4,R12,LR} (qword)
        "    ldmia r1!, {r3, r4, r12, lr}    \n"  //     {R3,R4,R12,LR} = *src++ (qword)
        "    stmia r0!, {r3, r4, r12, lr}    \n"  //     *dest++ = {R3,R4,R12,LR} (qword)
        "    subs r2, r2, #0x20              \n"  //     len -= 32
        "    bge 1b                          \n"  // WHILE len >= 0
        // Less than 32 bytes remaining, copy 16 if enough are remaining
        "    cmn r2, #0x10                   \n"  // TEST len + 16
        "    ittt ge                         \n"  // IF len >= -16:
        "    ldmiage r1!, {r3, r4, r12, lr}  \n"  //     THEN {R3,R4,R12,LR} = *src++ (qword)
        "    stmiage r0!, {r3, r4, r12, lr}  \n"  //     THEN *dest++ = {R3,R4,R12,LR} (qword)
        "    subge r2, r2, #0x10             \n"  //     THEN len -= 16
        // No need for R4 anymore, restore it so that later code doesn't have to take care of it
        "    ldr r4, [sp], #4                \n"  // RESTORE R4                                         // STACK: orig_dest return_addr
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "4:                                  \n"  // [remainder31]: we have less than 32 bytes remaining
        // Correct length offset from 32 byte copying mode
        "    adds r2, r2, #0x14              \n"  // len += 20                                          // len offset: -12
        "1:                                  \n"  // DO:
        // Copy 12 bytes at a time, while enough are remaining
        "    itttt ge                        \n"  //     IF len >= 0:
        "    ldmiage r1!, {r3, r12, lr}      \n"  //         THEN {R3,R12,LR} = *src++ (12 bytes)
        "    stmiage r0!, {r3, r12, lr}      \n"  //         THEN *dest++ = {R3,R12,LR} (12 bytes)
        "    subsge r2, r2, #0x0c            \n"  //         THEN len -= 12
        "    bge 1b                          \n"  // WHILE len >= 12
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "5:                                  \n"  // [remainder11]: we have less than 12 bytes remaining
        // Correct length offset from 12 byte copying mode
        "    adds r2, r2, #8                 \n"  // len += 8                                           // len offset: -4
        // Skip to 3 byte tail if less than 4 bytes are remaining
        "    blt 6f                          \n"  // IF len < 0: THEN GOTO [remainder3]
        // We will copy at least 4 bytes, adjust length
        "    subs r2, r2, #4                 \n"  // len -= 4
        // If less than 8 bytes are remaining, copy 4 bytes
        "    itt lt                          \n"  // IF len < 0:
        "    ldrlt r3, [r1], #4              \n"  //     THEN R3 = *src++ (word)
        "    strlt r3, [r0], #4              \n"  //     THEN *dest++ = R3 (word)
        // If at least 8 bytes are remaining, copy 8 bytes
        "    ittt ge                         \n"  // IF len >= 0:
        "    ldmiage r1!, {r3, r12}          \n"  //     THEN {R3,R12} = *src++ (dword)
        "    stmiage r0!, {r3, r12}          \n"  //     THEN *dest++ = {R3,R12} (dword)
        // We have subtracted 4 bytes above but copied 8, adjust length
        "    subge r2, r2, #4                \n"  //     THEN len -= 4
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "6:                                  \n"  // [remainder3]: we have less than 4 bytes remaining, copy them individually
        // Correct length offset from 4 byte copying mode
        "    adds r2, r2, #4                 \n"  // len += 4                                           // len offset: 0
        // If we're finished, return
        "    it eq                           \n"  // IF len == 0:
        "    ldmfdeq sp!, {r0, pc}           \n"  //     THEN RETURN orig_dest
        "    cmp r2, #2                      \n"  // TEST len - 2
        // We always have to copy at least one byte
        "    ldrb r3, [r1], #1               \n"  // R3 = *src++ (byte)
        "    strb r3, [r0], #1               \n"  // *dest++ = R3 (byte)
        // If we have to copy at least two, copy another one
        "    itt ge                          \n"  // IF len >= 2:
        "    ldrbge r3, [r1], #1             \n"  //     THEN R3 = *src++ (byte)
        "    strbge r3, [r0], #1             \n"  //     THEN *dest++ = R3 (byte)
        // If we have to copy more than two (which is alweays 3), copy another one
        "    itt gt                          \n"  // IF len > 2:
        "    ldrbgt r3, [r1], #1             \n"  //     THEN R3 = *src++ (byte)
        "    strbgt r3, [r0], #1             \n"  //     THEN *dest++ = R3 (byte)
        // Everything copied, return
        "    ldmfd sp!, {r0, pc}             \n"  // RETURN orig_dest
        "                                    \n"  //
        "7:                                  \n"  // [align]: the destination is misaligned, align it
        // Negate the misalignment to figure out how much we have to adjust
        "    rsb r12, r12, #4                \n"  // misalignment = 4 - misalignment
        "    cmp r12, #2                     \n"  // TEST misalignment - 2
        // We always have to copy at least one byte
        "    ldrb r3, [r1], #1               \n"  // R3 = *src++ (byte)
        "    strb r3, [r0], #1               \n"  // *dest++ = R3 (byte)
        // If we have to copy at least two, copy another one
        "    itt ge                          \n"  // IF misalignment >= 2:
        "    ldrbge r3, [r1], #1             \n"  //     THEN R3 = *src++ (byte)
        "    strbge r3, [r0], #1             \n"  //     THEN *dest++ = R3 (byte)
        // If we have to copy more than two (which is alweays 3), copy another one
        "    itt gt                          \n"  // IF misalignment > 2:
        "    ldrbgt r3, [r1], #1             \n"  //     THEN R3 = *src++ (byte)
        "    strbgt r3, [r0], #1             \n"  //     THEN *dest++ = R3 (byte)
        // The destination is aligned now, check if there are at least 4 bytes remaining
        "    subs r2, r2, r12                \n"  // len -= misalignment
        // Skip to 3 byte tail if not
        "    blt 6b                          \n"  // IF len < 0: THEN GOTO [remainder3]
        // If the source is now misaligned, we have to copy byte by byte
        "    tst r1, #3                      \n"  // TEST src & 3
        // If not, resume fast copying method above
        "    beq 3b                          \n"  // IF !(src & 3): GOTO [aligned]
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "8:                                  \n"  // [bytewise]: the destination is aligned, but the source isn't, copy byte by byte
        "                                    \n"  // DO:
        // Stupidly copy a byte at a time
        "    ldrb r3, [r1], #1               \n"  //     R3 = *src++ (byte)
        "    strb r3, [r0], #1               \n"  //     *dest++ = R3 (byte)
        "    subs r2, r2, #1                 \n"  //     len--
        "    bge 8b                          \n"  // WHILE len >= 0
        // Less than 4 bytes remaining, use 3 byte tail copying code above
        "    b 6b                            \n"  // GOTO [remainder3]
        // End of performance optimized forward implementation
#endif

        "9:                                  \n"  // [backward]: the destination is above the source, so we need to copy backwards
        // Jump to end of src and dest, and copy backwards
        "    add r1, r1, r2                  \n"  // src += len
        "    add r0, r0, r2                  \n"  // dest += len
        "                                    \n"  // (fallthrough)

#if defined(SQUEEZE)
        // Absolute minimum size backward implementation:
        "1:                                  \n"  // DO:
        // Stupidly copy a byte at a time
        "    subs r2, r2, #1                 \n"  //     len--
        "    it lt                           \n"  //     IF len < 0:
        "    ldmfdlt sp!, {r0, pc}           \n"  //         THEN RETURN orig_dest
        "    ldrb r3, [r1, #-1]!             \n"  //     R3 = *--src (byte)
        "    strb r3, [r0, #-1]!             \n"  //     *--dest = R3 (byte)
        "    b 1b                            \n"  // WHILE true
        // End of absolute minimum size backward implementation

#elif defined(OPTIMIZE_SIZE)
        // Size optimized backward implementation:
        // Skip to tail if we have to copy less than 4 bytes
        "    subs r2, r2, #4                 \n"  // len -= 4                                           // len offset: -4
        "    blt 2f                          \n"  // IF len < 0: THEN GOTO [remainder]
        // Skip to tail if src or dest are misaligned
        "    tst r0, #3                      \n"  // TEST dest & 3
        "    it eq                           \n"  // IF !(dest & 3):
        "    tsteq r1, #3                    \n"  //     THEN TEST src & 3
        "    bne 2f                          \n"  // IF (dest & 3) OR (src & 3): THEN GOTO [remainder]
        // Copy 4 bytes at a time until less than 4 are remaining
        "1:                                  \n"  // DO:
        "    ldr r3, [r1, #-4]!              \n"  //     R3 = *--src (word)
        "    str r3, [r0, #-4]!              \n"  //     *--dest = R3 (word)
        "    subs r2, r2, #4                 \n"  //     len -= 4
        "    bge 1b                          \n"  // WHILE: len >= 0
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "2:                                  \n"  // [remainder]: copy the remainder byte by byte
        // Correct length offset from 4 byte copying mode
        "    adds r2, r2, #4                 \n"  // len += 4                                           // len offset: 0
        "1:                                  \n"  // DO:
        // Copy the remainder a byte at a time
        "    it eq                           \n"  //     IF len == 0:
        "    ldmfdeq sp!, {r0, pc}           \n"  //         THEN RETURN orig_dest
        "    ldrb r3, [r1, #-1]!             \n"  //     R3 = *--src (byte)
        "    strb r3, [r0, #-1]!             \n"  //     *--dest = R3 (byte)
        "    subs r2, r2, #1                 \n"  //     len--
        "    b 1b                            \n"  // WHILE true
        // End of size optimized backward implementation

#else
        // Performance optimized backward implementation:
        // Skip to 3 byte tail if we have to copy less than 4 bytes
        "    subs r2, r2, #4                 \n"  // len -= 4                                           // len offset: -4
        "    blt 6f                          \n"  // IF len < 0: THEN GOTO [remainder3]
        // If the destination is misaligned, align it
        // This will return to label 3 if the source is aligned after the destination was aligned
        "    ands r12, r0, #3                \n"  // misalignment = dest & 3
        "    bne 7f                          \n"  // IF misalignment != 0: THEN GOTO [align]
        // The destination was already aligned, but the source isn't
        // We have no option but to fall back to byte by byte copying
        "    tst r1, #3                      \n"  // TEST src & 3
        "    bne 8f                          \n"  // IF src & 3: THEN GOTO [bytewise]
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "3:                                  \n"  // [aligned]: we are at least 4 byte aligned
        // Skip to 11 byte tail if we have to copy less than 12 bytes
        "    subs r2, r2, #8                 \n"  // len -= 8                                           // len offset: -12
        "    blt 5f                          \n"  // IF len < 0: THEN GOTO [remainder11]
        // Skip to 31 byte tail if we have to copy less than 32 bytes
        "    subs r2, r2, #0x14              \n"  // len -= 20                                          // len offset: -32
        "    blt 4f                          \n"  // IF len < 0: THEN GOTO [remainder31]
        // Save R4 so that we have an additional copying scratchpad register
        "    str r4, [sp, #-4]!              \n"  // SAVE R4                                            // STACK: R4 orig_dest return_addr
        "1:                                  \n"  // DO:
        // Copy 32 bytes at a time
        "    ldmdb r1!, {r3, r4, r12, lr}    \n"  //     {R3,R4,R12,LR} = *--src (qword)
        "    stmdb r0!, {r3, r4, r12, lr}    \n"  //     *--dest = {R3,R4,R12,LR} (qword)
        "    ldmdb r1!, {r3, r4, r12, lr}    \n"  //     {R3,R4,R12,LR} = *--src (qword)
        "    stmdb r0!, {r3, r4, r12, lr}    \n"  //     *--dest = {R3,R4,R12,LR} (qword)
        "    subs r2, r2, #0x20              \n"  //     len -= 32
        "    bge 1b                          \n"  // WHILE len >= 0
        // Less than 32 bytes remaining, copy 16 if enough are remaining
        "    cmn r2, #0x10                   \n"  // TEST len + 16
        "    ittt ge                         \n"  // IF len >= -16:
        "    ldmdbge r1!, {r3, r4, r12, lr}  \n"  //     THEN {R3,R4,R12,LR} = *--src (qword)
        "    stmdbge r0!, {r3, r4, r12, lr}  \n"  //     THEN *--dest = {R3,R4,R12,LR} (qword)
        "    subge r2, r2, #0x10             \n"  //     THEN len -= 16
        // No need for R4 anymore, restore it so that later code doesn't have to take care of it
        "    ldr r4, [sp], #4                \n"  // RESTORE R4                                         // STACK: orig_dest return_addr
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "4:                                  \n"  // [remainder31]: we have less than 32 bytes remaining
        // Correct length offset from 32 byte copying mode
        "    adds r2, r2, #0x14              \n"  // len += 20                                          // len offset: -12
        "1:                                  \n"  // DO:
        // Copy 12 bytes at a time, while enough are remaining
        "    itttt ge                        \n"  //     IF len >= 0:
        "    ldmdbge r1!, {r3, r12, lr}      \n"  //         THEN {R3,R12,LR} = *--src (12 bytes)
        "    stmdbge r0!, {r3, r12, lr}      \n"  //         THEN *--dest = {R3,R12,LR} (12 bytes)
        "    subsge r2, r2, #0x0c            \n"  //         THEN len -= 12
        "    bge 1b                          \n"  // WHILE len >= 12
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "5:                                  \n"  // [remainder11]: we have less than 12 bytes remaining
        // Correct length offset from 12 byte copying mode
        "    adds r2, r2, #8                 \n"  // len += 8                                           // len offset: -4
        // Skip to 3 byte tail if less than 4 bytes are remaining
        "    blt 6f                          \n"  // IF len < 0: THEN GOTO [remainder3]
        // We will copy at least 4 bytes, adjust length
        "    subs r2, r2, #4                 \n"  // len -= 4
        // If less than 8 bytes are remaining, copy 4 bytes
        "    itt lt                          \n"  // IF len < 0:
        "    ldrlt r3, [r1, #-4]!            \n"  //     THEN R3 = *--src (word)
        "    strlt r3, [r0, #-4]!            \n"  //     THEN *--dest = R3 (word)
        // If at least 8 bytes are remaining, copy 8 bytes
        "    ittt ge                         \n"  // IF len >= 0:
        "    ldmdbge r1!, {r3, r12}          \n"  //     THEN {R3,R12} = *--src (dword)
        "    stmdbge r0!, {r3, r12}          \n"  //     THEN *--dest = {R3,R12} (dword)
        // We have subtracted 4 bytes above but copied 8, adjust length
        "    subge r2, r2, #4                \n"  //     THEN len -= 4
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "6:                                  \n"  // [remainder3]: we have less than 4 bytes remaining, copy them individually
        // Correct length offset from 4 byte copying mode
        "    adds r2, r2, #4                 \n"  // len += 4                                           // len offset: 0
        // If we're finished, return
        "    it eq                           \n"  // IF len == 0:
        "    ldmfdeq sp!, {r0, pc}           \n"  //     THEN RETURN orig_dest
        "    cmp r2, #2                      \n"  // TEST len - 2
        // We always have to copy at least one byte
        "    ldrb r3, [r1, #-1]!             \n"  // R3 = *--src (byte)
        "    strb r3, [r0, #-1]!             \n"  // *--dest = R3 (byte)
        // If we have to copy at least two, copy another one
        "    itt ge                          \n"  // IF len >= 2:
        "    ldrbge r3, [r1, #-1]!           \n"  //     THEN R3 = *--src (byte)
        "    strbge r3, [r0, #-1]!           \n"  //     THEN *--dest = R3 (byte)
        // If we have to copy more than two (which is alweays 3), copy another one
        "    itt gt                          \n"  // IF len > 2:
        "    ldrbgt r3, [r1, #-1]!           \n"  //     THEN R3 = *--src (byte)
        "    strbgt r3, [r0, #-1]!           \n"  //     THEN *--dest = R3 (byte)
        // Everything copied, return
        "    ldmfd sp!, {r0, pc}             \n"  // RETURN orig_dest
        "                                    \n"  //
        "7:                                  \n"  // [align]: the destination is misaligned, align it
        // No need to negate the misalignment here, we are going backwards so we have to adjust by the misaligned amount
        "    cmp r12, #2                     \n"  // TEST misalignment - 2
        // We always have to copy at least one byte
        "    ldrb r3, [r1, #-1]!             \n"  // R3 = *--src (byte)
        "    strb r3, [r0, #-1]!             \n"  // *--dest = R3 (byte)
        // If we have to copy at least two, copy another one
        "    itt ge                          \n"  // IF misalignment >= 2:
        "    ldrbge r3, [r1, #-1]!           \n"  //     THEN R3 = *--src (byte)
        "    strbge r3, [r0, #-1]!           \n"  //     THEN *--dest = R3 (byte)
        // If we have to copy more than two (which is alweays 3), copy another one
        "    itt gt                          \n"  // IF misalignment > 2:
        "    ldrbgt r3, [r1, #-1]!           \n"  //     THEN R3 = *--src (byte)
        "    strbgt r3, [r0, #-1]!           \n"  //     THEN *--dest = R3 (byte)
        // The destination is aligned now, check if there are at least 4 bytes remaining
        "    subs r2, r2, r12                \n"  // len -= misalignment
        // Skip to 3 byte tail if not
        "    blt 6b                          \n"  // IF len < 0: THEN GOTO [remainder3]
        // If the source is now misaligned, we have to copy byte by byte
        "    tst r1, #3                      \n"  // TEST src & 3
        // If not, resume fast copying method above
        "    beq 3b                          \n"  // IF !(src & 3): GOTO [aligned]
        "                                    \n"  // (fallthrough)
        "                                    \n"  //
        "8:                                  \n"  // [bytewise]: the destination is aligned, but the source isn't, copy byte by byte
        "                                    \n"  // DO:
        // Stupidly copy a byte at a time
        "    ldrb r3, [r1, #-1]!             \n"  //     R3 = *--src (byte)
        "    strb r3, [r0, #-1]!             \n"  //     *--dest = R3 (byte)
        "    subs r2, r2, #1                 \n"  //     len--
        "    bge 8b                          \n"  // WHILE len >= 0
        // Less than 4 bytes remaining, use 3 byte tail copying code above
        "    b 6b                            \n"  // GOTO [remainder3]
        // End of performance optimized backward implementation
#endif
    );
}

__attribute__((alias("memmove"))) void* memcpy(void* dst, const void* src, size_t len);

#ifdef CPU_ARM_CORTEX_M
__attribute__((const)) uint32_t swap32(uint32_t data)
{
    __asm__("rev %[data], %[data]" : [data] "+r" (data));
    return data;
}

__attribute__((const)) uint32_t swap16(uint32_t data)
{
    __asm__("rev16 %[data], %[data]" : [data] "+r" (data));
    return data;
}

__attribute__((const)) uint32_t reverse32(uint32_t data)
{
    __asm__("rbit %[data], %[data]" : [data] "+r" (data));
    return data;
}
#endif
