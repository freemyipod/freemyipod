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


    .section .text.__embios_armhelpers
    .global __clzsi2
    .type   __clzsi2, %function
    .global __aeabi_uidivmod
    .type   __aeabi_uidivmod,%function
    .global __aeabi_uidiv
    .type   __aeabi_uidiv,%function
    .set    __aeabi_uidiv,__aeabi_uidivmod
    .global __aeabi_idivmod
    .type   __aeabi_idivmod,%function
    .global __aeabi_idiv
    .type   __aeabi_idiv,%function
    .set    __aeabi_idiv,__aeabi_idivmod

__clzsi2:
    str lr, [sp,#-4]!
    swi 3
    ldr	lr, [sp], #4
    bx	lr
    .size __clzsi2, .-__clzsi2

__aeabi_uidivmod:
    str lr, [sp,#-4]!
    swi 4
    ldr	lr, [sp], #4
    bx	lr
    .size __aeabi_uidivmod, . - __aeabi_uidivmo

__aeabi_idivmod:
    str lr, [sp,#-4]!
    swi 5
    ldr	lr, [sp], #4
    bx	lr
    .size __aeabi_idivmod, . - __aeabi_idivmod
