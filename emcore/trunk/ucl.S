/* arm_nrv2e_d8.S -- ARM decompressor for NRV2E

   This file is part of the UPX executable compressor.

   Copyright (C) 1996-2008 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996-2008 Laszlo Molnar
   Copyright (C) 2000-2008 John F. Reiser
   All Rights Reserved.

   UPX and the UCL library are free software; you can redistribute them
   and/or modify them under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer              Laszlo Molnar
   <markus@oberhumer.com>               <ml1050@users.sourceforge.net>

   John F. Reiser
   <jreiser@users.sourceforge.net>
*/

#define src  r0
#define len  r1  /* overlaps 'cnt' */
#define dst  r2
#define tmp  r3
#define bits r4
#define off  r5
#define wrnk r6  /* 0x500  M2_MAX_OFFSET before "wrinkle" */
#define srclim r7

#define cnt  r1  /* overlaps 'len' while reading an offset */

/* "mov lr,pc; bxx ..." implements conditional subroutine call */
#define GETBIT  add bits,bits; mov lr,pc; beq get1_n2e

#define getnextb(reg) GETBIT; adc reg,reg
#define   jnextb0     GETBIT; bcc
#define   jnextb1     GETBIT; bcs

.section .icode.ucl_decompress, "ax", %progbits
.align 2
ucl_decompress: .globl ucl_decompress  @ ARM mode
        .type ucl_decompress, %function
/* error = (*)(char const *src, int len_src, char *dst, int *plen_dst)
   Actual decompressed length is stored through plen_dst.
*/
        adr r12,1+.thumb_nrv2e_d8; bx r12  @ enter THUMB mode
        .code 16  @ THUMB mode
        .thumb_func

.thumb_nrv2e_d8:
        push {r2,r3,r4,r5,r6,r7,lr}
        add srclim,len,src  @ srclim= eof_src;
        mov bits,#1; neg off,bits  @ off= -1 initial condition
        lsl bits,#31  @ 1<<31: refill next time
        mov wrnk,#5
        lsl wrnk,#8  @ 0x500  @ nrv2e M2_MAX_OFFSET
        b top_n2e

eof_n2e:
        pop {r3,r4}  @ r3= orig_dst; r4= plen_dst
        sub src,srclim  @ 0 if actual src length equals expected length
        sub dst,r3  @ actual dst length
        str dst,[r4]
        pop {r4,r5,r6,r7}
        pop {r1}
        bx r1

get1_n2e:  @ In: Carry set [from adding 0x80000000 (1<<31) to itself]
          ldrb bits,[src]  @ zero-extend next byte
        adc bits,bits  @ double and insert CarryIn as low bit
          add src,#1
        lsl bits,#24  @ move to top byte, and set CarryOut from old bit 8
        mov pc,lr  @ return, stay in current (THUMB) mode

lit_n2e:
        ldrb tmp,[src]; add src,#1
        strb tmp,[dst]; add dst,#1
top_n2e:
        jnextb1 lit_n2e
        mov cnt,#1; b getoff_n2e

off_n2e:
        sub cnt,#1
        getnextb(cnt)
getoff_n2e:
        getnextb(cnt)
        jnextb0 off_n2e

        sub tmp,cnt,#3  @ set Carry
        mov len,#0  @ Carry unaffected
        blo offprev_n2e  @ cnt was 2; tests Carry only
        lsl tmp,#8
        ldrb off,[src]; add src,#1  @ low 7+1 bits
        orr  off,tmp
        mvn  off,off; beq eof_n2e  @ off= ~off
        asr  off,#1; bcs lenlast_n2e
        b lenmore_n2e

offprev_n2e:
        jnextb1 lenlast_n2e
lenmore_n2e:
        mov len,#1
        jnextb1 lenlast_n2e
len_n2e:
        getnextb(len)
        jnextb0 len_n2e
        add len,#6-2
        b gotlen_n2e

lenlast_n2e:
        getnextb(len)  @ 0,1,2,3
        add len,#2
gotlen_n2e:  @ 'cmn': add the inputs, set condition codes, discard the sum
        cmn wrnk,off; bcs near_n2e  @ within M2_MAX_OFFSET
        add len,#1  @ too far away, so minimum match length is 3
near_n2e:
        ldrb tmp,[dst]  @ force cacheline allocate
copy_n2e:
        ldrb tmp,[dst,off]
        strb tmp,[dst]; add dst,#1
        sub len,#1; bne copy_n2e
        b top_n2e

        .size ucl_decompress, .-ucl_decompress
