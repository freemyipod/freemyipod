//
//
//    Copyright 2011 TheSeven
//
//
//    This file is part of emCORE.
//
//    emCORE is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emCORE is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include "emcoreapp.h"
#undef memset
#undef memmove
#undef memcpy
#undef memcmp


int __clzsi2(int arg) __attribute__((naked,noinline));
int __clzsi2(int arg)
{
    __asm__ volatile("ldr\tr12, =__emcore_syscall\n\tldr\tr12, [r12]\n\tldr\tpc, [r12,#8]\n\t.ltorg\n\t");
}

// This one has a non-standard calling convention, so we just declare it as void
void __aeabi_idivmod(void) __attribute__((naked,noinline));
void __aeabi_idivmod(void)
{
    __asm__ volatile("ldr\tr12, =__emcore_syscall\n\tldr\tr12, [r12]\n\tldr\tpc, [r12,#0xc]\n\t.ltorg\n\t");
}

// This one has a non-standard calling convention, so we just declare it as void
void __aeabi_uidivmod(void) __attribute__((naked,noinline));
void __aeabi_uidivmod(void)
{
    __asm__ volatile("ldr\tr12, =__emcore_syscall\n\tldr\tr12, [r12]\n\tldr\tpc, [r12,#0x10]\n\t.ltorg\n\t");
}

extern __attribute__((alias("__aeabi_idivmod"))) void __aeabi_idiv(void);
extern __attribute__((alias("__aeabi_uidivmod"))) void __aeabi_uidiv(void);

void* memset(void *dst, int c, size_t length)
{
    return __emcore_syscall->memset(dst, c, length);
}

void* memmove(void *dst, const void *src, size_t length)
{
    return __emcore_syscall->memmove(dst, src, length);
}

void* memcpy(void *dst, const void *src, size_t length)
{
    return __emcore_syscall->memcpy(dst, src, length);
}

int memcmp(const void *src1, const void *src2, size_t length)
{
    return __emcore_syscall->memcmp(src1, src2, length);
}
