//
//
//    Copyright 2010 TheSeven
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


#ifndef __S5L8720_H__
#define __S5L8720_H__

#include "global.h"


/////SYSCON/////
#define PWRCON(i)    (*((uint32_t volatile*)(0x3C500000 \
                                           + ((i) == 4 ? 0x6C : \
                                             ((i) == 3 ? 0x68 : \
                                             ((i) == 2 ? 0x58 : \
                                             ((i) == 1 ? 0x4C : \
                                                         0x48)))))))


/////TIMER/////
#define TACON        (*((uint32_t volatile*)(0x3C700000)))
#define TACMD        (*((uint32_t volatile*)(0x3C700004)))
#define TADATA0      (*((uint32_t volatile*)(0x3C700008)))
#define TADATA1      (*((uint32_t volatile*)(0x3C70000C)))
#define TAPRE        (*((uint32_t volatile*)(0x3C700010)))
#define TACNT        (*((uint32_t volatile*)(0x3C700014)))
#define TBCON        (*((uint32_t volatile*)(0x3C700020)))
#define TBCMD        (*((uint32_t volatile*)(0x3C700024)))
#define TBDATA0      (*((uint32_t volatile*)(0x3C700028)))
#define TBDATA1      (*((uint32_t volatile*)(0x3C70002C)))
#define TBPRE        (*((uint32_t volatile*)(0x3C700030)))
#define TBCNT        (*((uint32_t volatile*)(0x3C700034)))
#define TCCON        (*((uint32_t volatile*)(0x3C700040)))
#define TCCMD        (*((uint32_t volatile*)(0x3C700044)))
#define TCDATA0      (*((uint32_t volatile*)(0x3C700048)))
#define TCDATA1      (*((uint32_t volatile*)(0x3C70004C)))
#define TCPRE        (*((uint32_t volatile*)(0x3C700050)))
#define TCCNT        (*((uint32_t volatile*)(0x3C700054)))
#define TDCON        (*((uint32_t volatile*)(0x3C700060)))
#define TDCMD        (*((uint32_t volatile*)(0x3C700064)))
#define TDDATA0      (*((uint32_t volatile*)(0x3C700068)))
#define TDDATA1      (*((uint32_t volatile*)(0x3C70006C)))
#define TDPRE        (*((uint32_t volatile*)(0x3C700070)))
#define TDCNT        (*((uint32_t volatile*)(0x3C700074)))
#define TECON        (*((uint32_t volatile*)(0x3C7000A0)))
#define TECMD        (*((uint32_t volatile*)(0x3C7000A4)))
#define TEDATA0      (*((uint32_t volatile*)(0x3C7000A8)))
#define TEDATA1      (*((uint32_t volatile*)(0x3C7000AC)))
#define TEPRE        (*((uint32_t volatile*)(0x3C7000B0)))
#define TECNT        (*((uint32_t volatile*)(0x3C7000B4)))
#define TFCON        (*((uint32_t volatile*)(0x3C7000C0)))
#define TFCMD        (*((uint32_t volatile*)(0x3C7000C4)))
#define TFDATA0      (*((uint32_t volatile*)(0x3C7000C8)))
#define TFDATA1      (*((uint32_t volatile*)(0x3C7000CC)))
#define TFPRE        (*((uint32_t volatile*)(0x3C7000D0)))
#define TFCNT        (*((uint32_t volatile*)(0x3C7000D4)))
#define TGCON        (*((uint32_t volatile*)(0x3C7000E0)))
#define TGCMD        (*((uint32_t volatile*)(0x3C7000E4)))
#define TGDATA0      (*((uint32_t volatile*)(0x3C7000E8)))
#define TGDATA1      (*((uint32_t volatile*)(0x3C7000EC)))
#define TGPRE        (*((uint32_t volatile*)(0x3C7000F0)))
#define TGCNT        (*((uint32_t volatile*)(0x3C7000F4)))
#define THCON        (*((uint32_t volatile*)(0x3C700100)))
#define THCMD        (*((uint32_t volatile*)(0x3C700104)))
#define THDATA0      (*((uint32_t volatile*)(0x3C700108)))
#define THDATA1      (*((uint32_t volatile*)(0x3C70010C)))
#define THPRE        (*((uint32_t volatile*)(0x3C700110)))
#define THCNT        (*((uint32_t volatile*)(0x3C700114)))


/////USB/////
#define OTGBASE 0x38400000
#define PHYBASE 0x3C400000
#define SYNOPSYSOTG_CLOCK 0x11
#define SYNOPSYSOTG_AHBCFG 0x2B


/////I2C/////
#define IICCON(bus)  (*((uint32_t volatile*)(0x3C600000 + 0x300000 * (bus))))
#define IICSTAT(bus) (*((uint32_t volatile*)(0x3C600004 + 0x300000 * (bus))))
#define IICADD(bus)  (*((uint32_t volatile*)(0x3C600008 + 0x300000 * (bus))))
#define IICDS(bus)   (*((uint32_t volatile*)(0x3C60000C + 0x300000 * (bus))))


/////INTERRUPT CONTROLLERS/////
#define VICIRQSTATUS(v)       (*((uint32_t volatile*)(0x38E00000 + 0x1000 * (v))))
#define VICFIQSTATUS(v)       (*((uint32_t volatile*)(0x38E00004 + 0x1000 * (v))))
#define VICRAWINTR(v)         (*((uint32_t volatile*)(0x38E00008 + 0x1000 * (v))))
#define VICINTSELECT(v)       (*((uint32_t volatile*)(0x38E0000C + 0x1000 * (v))))
#define VICINTENABLE(v)       (*((uint32_t volatile*)(0x38E00010 + 0x1000 * (v))))
#define VICINTENCLEAR(v)      (*((uint32_t volatile*)(0x38E00014 + 0x1000 * (v))))
#define VICSOFTINT(v)         (*((uint32_t volatile*)(0x38E00018 + 0x1000 * (v))))
#define VICSOFTINTCLEAR(v)    (*((uint32_t volatile*)(0x38E0001C + 0x1000 * (v))))
#define VICPROTECTION(v)      (*((uint32_t volatile*)(0x38E00020 + 0x1000 * (v))))
#define VICSWPRIORITYMASK(v)  (*((uint32_t volatile*)(0x38E00024 + 0x1000 * (v))))
#define VICPRIORITYDAISY(v)   (*((uint32_t volatile*)(0x38E00028 + 0x1000 * (v))))
#define VICVECTADDR(v, i)     (*((uint32_t volatile*)(0x38E00100 + 0x1000 * (v) + 4 * (i))))
#define VICVECTPRIORITY(v, i) (*((uint32_t volatile*)(0x38E00200 + 0x1000 * (v) + 4 * (i))))
#define VICADDRESS(v)         (*((const void* volatile*)(0x38E00F00 + 0x1000 * (v))))
#define VIC0IRQSTATUS         (*((uint32_t volatile*)(0x38E00000)))
#define VIC0FIQSTATUS         (*((uint32_t volatile*)(0x38E00004)))
#define VIC0RAWINTR           (*((uint32_t volatile*)(0x38E00008)))
#define VIC0INTSELECT         (*((uint32_t volatile*)(0x38E0000C)))
#define VIC0INTENABLE         (*((uint32_t volatile*)(0x38E00010)))
#define VIC0INTENCLEAR        (*((uint32_t volatile*)(0x38E00014)))
#define VIC0SOFTINT           (*((uint32_t volatile*)(0x38E00018)))
#define VIC0SOFTINTCLEAR      (*((uint32_t volatile*)(0x38E0001C)))
#define VIC0PROTECTION        (*((uint32_t volatile*)(0x38E00020)))
#define VIC0SWPRIORITYMASK    (*((uint32_t volatile*)(0x38E00024)))
#define VIC0PRIORITYDAISY     (*((uint32_t volatile*)(0x38E00028)))
#define VIC0VECTADDR(i)       (*((const void* volatile*)(0x38E00100 + 4 * (i))))
#define VIC0VECTADDR0         (*((const void* volatile*)(0x38E00100)))
#define VIC0VECTADDR1         (*((const void* volatile*)(0x38E00104)))
#define VIC0VECTADDR2         (*((const void* volatile*)(0x38E00108)))
#define VIC0VECTADDR3         (*((const void* volatile*)(0x38E0010C)))
#define VIC0VECTADDR4         (*((const void* volatile*)(0x38E00110)))
#define VIC0VECTADDR5         (*((const void* volatile*)(0x38E00114)))
#define VIC0VECTADDR6         (*((const void* volatile*)(0x38E00118)))
#define VIC0VECTADDR7         (*((const void* volatile*)(0x38E0011C)))
#define VIC0VECTADDR8         (*((const void* volatile*)(0x38E00120)))
#define VIC0VECTADDR9         (*((const void* volatile*)(0x38E00124)))
#define VIC0VECTADDR10        (*((const void* volatile*)(0x38E00128)))
#define VIC0VECTADDR11        (*((const void* volatile*)(0x38E0012C)))
#define VIC0VECTADDR12        (*((const void* volatile*)(0x38E00130)))
#define VIC0VECTADDR13        (*((const void* volatile*)(0x38E00134)))
#define VIC0VECTADDR14        (*((const void* volatile*)(0x38E00138)))
#define VIC0VECTADDR15        (*((const void* volatile*)(0x38E0013C)))
#define VIC0VECTADDR16        (*((const void* volatile*)(0x38E00140)))
#define VIC0VECTADDR17        (*((const void* volatile*)(0x38E00144)))
#define VIC0VECTADDR18        (*((const void* volatile*)(0x38E00148)))
#define VIC0VECTADDR19        (*((const void* volatile*)(0x38E0014C)))
#define VIC0VECTADDR20        (*((const void* volatile*)(0x38E00150)))
#define VIC0VECTADDR21        (*((const void* volatile*)(0x38E00154)))
#define VIC0VECTADDR22        (*((const void* volatile*)(0x38E00158)))
#define VIC0VECTADDR23        (*((const void* volatile*)(0x38E0015C)))
#define VIC0VECTADDR24        (*((const void* volatile*)(0x38E00160)))
#define VIC0VECTADDR25        (*((const void* volatile*)(0x38E00164)))
#define VIC0VECTADDR26        (*((const void* volatile*)(0x38E00168)))
#define VIC0VECTADDR27        (*((const void* volatile*)(0x38E0016C)))
#define VIC0VECTADDR28        (*((const void* volatile*)(0x38E00170)))
#define VIC0VECTADDR29        (*((const void* volatile*)(0x38E00174)))
#define VIC0VECTADDR30        (*((const void* volatile*)(0x38E00178)))
#define VIC0VECTADDR31        (*((const void* volatile*)(0x38E0017C)))
#define VIC0VECTPRIORITY(i)   (*((uint32_t volatile*)(0x38E00200 + 4 * (i))))
#define VIC0VECTPRIORITY0     (*((uint32_t volatile*)(0x38E00200)))
#define VIC0VECTPRIORITY1     (*((uint32_t volatile*)(0x38E00204)))
#define VIC0VECTPRIORITY2     (*((uint32_t volatile*)(0x38E00208)))
#define VIC0VECTPRIORITY3     (*((uint32_t volatile*)(0x38E0020C)))
#define VIC0VECTPRIORITY4     (*((uint32_t volatile*)(0x38E00210)))
#define VIC0VECTPRIORITY5     (*((uint32_t volatile*)(0x38E00214)))
#define VIC0VECTPRIORITY6     (*((uint32_t volatile*)(0x38E00218)))
#define VIC0VECTPRIORITY7     (*((uint32_t volatile*)(0x38E0021C)))
#define VIC0VECTPRIORITY8     (*((uint32_t volatile*)(0x38E00220)))
#define VIC0VECTPRIORITY9     (*((uint32_t volatile*)(0x38E00224)))
#define VIC0VECTPRIORITY10    (*((uint32_t volatile*)(0x38E00228)))
#define VIC0VECTPRIORITY11    (*((uint32_t volatile*)(0x38E0022C)))
#define VIC0VECTPRIORITY12    (*((uint32_t volatile*)(0x38E00230)))
#define VIC0VECTPRIORITY13    (*((uint32_t volatile*)(0x38E00234)))
#define VIC0VECTPRIORITY14    (*((uint32_t volatile*)(0x38E00238)))
#define VIC0VECTPRIORITY15    (*((uint32_t volatile*)(0x38E0023C)))
#define VIC0VECTPRIORITY16    (*((uint32_t volatile*)(0x38E00240)))
#define VIC0VECTPRIORITY17    (*((uint32_t volatile*)(0x38E00244)))
#define VIC0VECTPRIORITY18    (*((uint32_t volatile*)(0x38E00248)))
#define VIC0VECTPRIORITY19    (*((uint32_t volatile*)(0x38E0024C)))
#define VIC0VECTPRIORITY20    (*((uint32_t volatile*)(0x38E00250)))
#define VIC0VECTPRIORITY21    (*((uint32_t volatile*)(0x38E00254)))
#define VIC0VECTPRIORITY22    (*((uint32_t volatile*)(0x38E00258)))
#define VIC0VECTPRIORITY23    (*((uint32_t volatile*)(0x38E0025C)))
#define VIC0VECTPRIORITY24    (*((uint32_t volatile*)(0x38E00260)))
#define VIC0VECTPRIORITY25    (*((uint32_t volatile*)(0x38E00264)))
#define VIC0VECTPRIORITY26    (*((uint32_t volatile*)(0x38E00268)))
#define VIC0VECTPRIORITY27    (*((uint32_t volatile*)(0x38E0026C)))
#define VIC0VECTPRIORITY28    (*((uint32_t volatile*)(0x38E00270)))
#define VIC0VECTPRIORITY29    (*((uint32_t volatile*)(0x38E00274)))
#define VIC0VECTPRIORITY30    (*((uint32_t volatile*)(0x38E00278)))
#define VIC0VECTPRIORITY31    (*((uint32_t volatile*)(0x38E0027C)))
#define VIC0ADDRESS           (*((void* volatile*)(0x38E00F00)))
#define VIC1IRQSTATUS         (*((uint32_t volatile*)(0x38E01000)))
#define VIC1FIQSTATUS         (*((uint32_t volatile*)(0x38E01004)))
#define VIC1RAWINTR           (*((uint32_t volatile*)(0x38E01008)))
#define VIC1INTSELECT         (*((uint32_t volatile*)(0x38E0100C)))
#define VIC1INTENABLE         (*((uint32_t volatile*)(0x38E01010)))
#define VIC1INTENCLEAR        (*((uint32_t volatile*)(0x38E01014)))
#define VIC1SOFTINT           (*((uint32_t volatile*)(0x38E01018)))
#define VIC1SOFTINTCLEAR      (*((uint32_t volatile*)(0x38E0101C)))
#define VIC1PROTECTION        (*((uint32_t volatile*)(0x38E01020)))
#define VIC1SWPRIORITYMASK    (*((uint32_t volatile*)(0x38E01024)))
#define VIC1PRIORITYDAISY     (*((uint32_t volatile*)(0x38E01028)))
#define VIC1VECTADDR(i)       (*((const void* volatile*)(0x38E01100 + 4 * (i))))
#define VIC1VECTADDR0         (*((const void* volatile*)(0x38E01100)))
#define VIC1VECTADDR1         (*((const void* volatile*)(0x38E01104)))
#define VIC1VECTADDR2         (*((const void* volatile*)(0x38E01108)))
#define VIC1VECTADDR3         (*((const void* volatile*)(0x38E0110C)))
#define VIC1VECTADDR4         (*((const void* volatile*)(0x38E01110)))
#define VIC1VECTADDR5         (*((const void* volatile*)(0x38E01114)))
#define VIC1VECTADDR6         (*((const void* volatile*)(0x38E01118)))
#define VIC1VECTADDR7         (*((const void* volatile*)(0x38E0111C)))
#define VIC1VECTADDR8         (*((const void* volatile*)(0x38E01120)))
#define VIC1VECTADDR9         (*((const void* volatile*)(0x38E01124)))
#define VIC1VECTADDR10        (*((const void* volatile*)(0x38E01128)))
#define VIC1VECTADDR11        (*((const void* volatile*)(0x38E0112C)))
#define VIC1VECTADDR12        (*((const void* volatile*)(0x38E01130)))
#define VIC1VECTADDR13        (*((const void* volatile*)(0x38E01134)))
#define VIC1VECTADDR14        (*((const void* volatile*)(0x38E01138)))
#define VIC1VECTADDR15        (*((const void* volatile*)(0x38E0113C)))
#define VIC1VECTADDR16        (*((const void* volatile*)(0x38E01140)))
#define VIC1VECTADDR17        (*((const void* volatile*)(0x38E01144)))
#define VIC1VECTADDR18        (*((const void* volatile*)(0x38E01148)))
#define VIC1VECTADDR19        (*((const void* volatile*)(0x38E0114C)))
#define VIC1VECTADDR20        (*((const void* volatile*)(0x38E01150)))
#define VIC1VECTADDR21        (*((const void* volatile*)(0x38E01154)))
#define VIC1VECTADDR22        (*((const void* volatile*)(0x38E01158)))
#define VIC1VECTADDR23        (*((const void* volatile*)(0x38E0115C)))
#define VIC1VECTADDR24        (*((const void* volatile*)(0x38E01160)))
#define VIC1VECTADDR25        (*((const void* volatile*)(0x38E01164)))
#define VIC1VECTADDR26        (*((const void* volatile*)(0x38E01168)))
#define VIC1VECTADDR27        (*((const void* volatile*)(0x38E0116C)))
#define VIC1VECTADDR28        (*((const void* volatile*)(0x38E01170)))
#define VIC1VECTADDR29        (*((const void* volatile*)(0x38E01174)))
#define VIC1VECTADDR30        (*((const void* volatile*)(0x38E01178)))
#define VIC1VECTADDR31        (*((const void* volatile*)(0x38E0117C)))
#define VIC1VECTPRIORITY(i)   (*((uint32_t volatile*)(0x38E01200 + 4 * (i))))
#define VIC1VECTPRIORITY0     (*((uint32_t volatile*)(0x38E01200)))
#define VIC1VECTPRIORITY1     (*((uint32_t volatile*)(0x38E01204)))
#define VIC1VECTPRIORITY2     (*((uint32_t volatile*)(0x38E01208)))
#define VIC1VECTPRIORITY3     (*((uint32_t volatile*)(0x38E0120C)))
#define VIC1VECTPRIORITY4     (*((uint32_t volatile*)(0x38E01210)))
#define VIC1VECTPRIORITY5     (*((uint32_t volatile*)(0x38E01214)))
#define VIC1VECTPRIORITY6     (*((uint32_t volatile*)(0x38E01218)))
#define VIC1VECTPRIORITY7     (*((uint32_t volatile*)(0x38E0121C)))
#define VIC1VECTPRIORITY8     (*((uint32_t volatile*)(0x38E01220)))
#define VIC1VECTPRIORITY9     (*((uint32_t volatile*)(0x38E01224)))
#define VIC1VECTPRIORITY10    (*((uint32_t volatile*)(0x38E01228)))
#define VIC1VECTPRIORITY11    (*((uint32_t volatile*)(0x38E0122C)))
#define VIC1VECTPRIORITY12    (*((uint32_t volatile*)(0x38E01230)))
#define VIC1VECTPRIORITY13    (*((uint32_t volatile*)(0x38E01234)))
#define VIC1VECTPRIORITY14    (*((uint32_t volatile*)(0x38E01238)))
#define VIC1VECTPRIORITY15    (*((uint32_t volatile*)(0x38E0123C)))
#define VIC1VECTPRIORITY16    (*((uint32_t volatile*)(0x38E01240)))
#define VIC1VECTPRIORITY17    (*((uint32_t volatile*)(0x38E01244)))
#define VIC1VECTPRIORITY18    (*((uint32_t volatile*)(0x38E01248)))
#define VIC1VECTPRIORITY19    (*((uint32_t volatile*)(0x38E0124C)))
#define VIC1VECTPRIORITY20    (*((uint32_t volatile*)(0x38E01250)))
#define VIC1VECTPRIORITY21    (*((uint32_t volatile*)(0x38E01254)))
#define VIC1VECTPRIORITY22    (*((uint32_t volatile*)(0x38E01258)))
#define VIC1VECTPRIORITY23    (*((uint32_t volatile*)(0x38E0125C)))
#define VIC1VECTPRIORITY24    (*((uint32_t volatile*)(0x38E01260)))
#define VIC1VECTPRIORITY25    (*((uint32_t volatile*)(0x38E01264)))
#define VIC1VECTPRIORITY26    (*((uint32_t volatile*)(0x38E01268)))
#define VIC1VECTPRIORITY27    (*((uint32_t volatile*)(0x38E0126C)))
#define VIC1VECTPRIORITY28    (*((uint32_t volatile*)(0x38E01270)))
#define VIC1VECTPRIORITY29    (*((uint32_t volatile*)(0x38E01274)))
#define VIC1VECTPRIORITY30    (*((uint32_t volatile*)(0x38E01278)))
#define VIC1VECTPRIORITY31    (*((uint32_t volatile*)(0x38E0127C)))
#define VIC1ADDRESS           (*((void* volatile*)(0x38E01F00)))


/////GPIO/////
#define PCON(i)       (*((uint32_t volatile*)(0x3cf00000 + ((i) << 5))))
#define PDAT(i)       (*((uint32_t volatile*)(0x3cf00004 + ((i) << 5))))
#define PCON0         (*((uint32_t volatile*)(0x3cf00000)))
#define PDAT0         (*((uint32_t volatile*)(0x3cf00004)))
#define PCON1         (*((uint32_t volatile*)(0x3cf00020)))
#define PDAT1         (*((uint32_t volatile*)(0x3cf00024)))
#define PCON2         (*((uint32_t volatile*)(0x3cf00040)))
#define PDAT2         (*((uint32_t volatile*)(0x3cf00044)))
#define PCON3         (*((uint32_t volatile*)(0x3cf00060)))
#define PDAT3         (*((uint32_t volatile*)(0x3cf00064)))
#define PCON4         (*((uint32_t volatile*)(0x3cf00080)))
#define PDAT4         (*((uint32_t volatile*)(0x3cf00084)))
#define PCON5         (*((uint32_t volatile*)(0x3cf000a0)))
#define PDAT5         (*((uint32_t volatile*)(0x3cf000a4)))
#define PCON6         (*((uint32_t volatile*)(0x3cf000c0)))
#define PDAT6         (*((uint32_t volatile*)(0x3cf000c4)))
#define PCON7         (*((uint32_t volatile*)(0x3cf000e0)))
#define PDAT7         (*((uint32_t volatile*)(0x3cf000e4)))
#define PCON8         (*((uint32_t volatile*)(0x3cf00100)))
#define PDAT8         (*((uint32_t volatile*)(0x3cf00104)))
#define PCON9         (*((uint32_t volatile*)(0x3cf00120)))
#define PDAT9         (*((uint32_t volatile*)(0x3cf00124)))
#define PCONA         (*((uint32_t volatile*)(0x3cf00140)))
#define PDATA         (*((uint32_t volatile*)(0x3cf00144)))
#define PCONB         (*((uint32_t volatile*)(0x3cf00160)))
#define PDATB         (*((uint32_t volatile*)(0x3cf00164)))
#define PCONC         (*((uint32_t volatile*)(0x3cf00180)))
#define PDATC         (*((uint32_t volatile*)(0x3cf00184)))
#define PCOND         (*((uint32_t volatile*)(0x3cf001a0)))
#define PDATD         (*((uint32_t volatile*)(0x3cf001a4)))
#define PCONE         (*((uint32_t volatile*)(0x3cf001c0)))
#define PDATE         (*((uint32_t volatile*)(0x3cf001c4)))
#define PCONF         (*((uint32_t volatile*)(0x3cf001e0)))
#define PDATF         (*((uint32_t volatile*)(0x3cf001e4)))
#define GPIOCMD       (*((uint32_t volatile*)(0x3cf00200)))


/////SPI/////
#define SPIBASE(i)      ((i) == 2 ? 0x3d200000 : \
                         (i) == 1 ? 0x3ce00000 : \
                                    0x3c300000)
#define SPICLKGATE(i)   ((i) == 2 ? 0x2f : \
                         (i) == 1 ? 0x2b : \
                                    0x22)
#define SPIDMA(i)       ((i) == 2 ? 0xd : \
                         (i) == 1 ? 0xf : \
                                    0x5)
#define SPICTRL(i)    (*((uint32_t volatile*)(SPIBASE(i))))
#define SPISETUP(i)   (*((uint32_t volatile*)(SPIBASE(i) + 0x4)))
#define SPISTATUS(i)  (*((uint32_t volatile*)(SPIBASE(i) + 0x8)))
#define SPIUNKREG1(i) (*((uint32_t volatile*)(SPIBASE(i) + 0xc)))
#define SPITXDATA(i)  (*((uint32_t volatile*)(SPIBASE(i) + 0x10)))
#define SPIRXDATA(i)  (*((uint32_t volatile*)(SPIBASE(i) + 0x20)))
#define SPICLKDIV(i)  (*((uint32_t volatile*)(SPIBASE(i) + 0x30)))
#define SPIRXLIMIT(i) (*((uint32_t volatile*)(SPIBASE(i) + 0x34)))
#define SPIUNKREG3(i) (*((uint32_t volatile*)(SPIBASE(i) + 0x38)))


/////AES/////
#define AESCONTROL    (*((uint32_t volatile*)(0x38c00000)))
#define AESGO         (*((uint32_t volatile*)(0x38c00004)))
#define AESUNKREG0    (*((uint32_t volatile*)(0x38c00008)))
#define AESSTATUS     (*((uint32_t volatile*)(0x38c0000c)))
#define AESUNKREG1    (*((uint32_t volatile*)(0x38c00010)))
#define AESKEYLEN     (*((uint32_t volatile*)(0x38c00014)))
#define AESOUTSIZE    (*((uint32_t volatile*)(0x38c00018)))
#define AESOUTADDR    (*((void* volatile*)(0x38c00020)))
#define AESINSIZE     (*((uint32_t volatile*)(0x38c00024)))
#define AESINADDR     (*((const void* volatile*)(0x38c00028)))
#define AESAUXSIZE    (*((uint32_t volatile*)(0x38c0002c)))
#define AESAUXADDR    (*((void* volatile*)(0x38c00030)))
#define AESSIZE3      (*((uint32_t volatile*)(0x38c00034)))
#define AESKEY          ((uint32_t volatile*)(0x38c0004c))
#define AESTYPE       (*((uint32_t volatile*)(0x38c0006c)))
#define AESIV           ((uint32_t volatile*)(0x38c00074))
#define AESTYPE2      (*((uint32_t volatile*)(0x38c00088)))
#define AESUNKREG2    (*((uint32_t volatile*)(0x38c0008c)))


/////SHA1/////
#define SHA1CONFIG    (*((uint32_t volatile*)(0x38000000)))
#define SHA1RESET     (*((uint32_t volatile*)(0x38000004)))
#define SHA1RESULT      ((uint32_t volatile*)(0x38000020))
#define SHA1DATAIN      ((uint32_t volatile*)(0x38000040))


/////DMA/////
struct dma_lli
{
    void* srcaddr;
    void* dstaddr;
    const struct dma_lli* nextlli;
    uint32_t control;
};
#define DMACINTSTS(d)       (*((uint32_t volatile*)(0x38200000 + 0x1700000 * (d))))
#define DMACINTTCSTS(d)     (*((uint32_t volatile*)(0x38200004 + 0x1700000 * (d))))
#define DMACINTTCCLR(d)     (*((uint32_t volatile*)(0x38200008 + 0x1700000 * (d))))
#define DMACINTERRSTS(d)    (*((uint32_t volatile*)(0x3820000c + 0x1700000 * (d))))
#define DMACINTERRCLR(d)    (*((uint32_t volatile*)(0x38200010 + 0x1700000 * (d))))
#define DMACRAWINTTCSTS(d)  (*((uint32_t volatile*)(0x38200014 + 0x1700000 * (d))))
#define DMACRAWINTERRSTS(d) (*((uint32_t volatile*)(0x38200018 + 0x1700000 * (d))))
#define DMACENABLEDCHANS(d) (*((uint32_t volatile*)(0x3820001c + 0x1700000 * (d))))
#define DMACSOFTBREQ(d)     (*((uint32_t volatile*)(0x38200020 + 0x1700000 * (d))))
#define DMACSOFTSREQ(d)     (*((uint32_t volatile*)(0x38200024 + 0x1700000 * (d))))
#define DMACSOFTLBREQ(d)    (*((uint32_t volatile*)(0x38200028 + 0x1700000 * (d))))
#define DMACSOFTLSREQ(d)    (*((uint32_t volatile*)(0x3820002c + 0x1700000 * (d))))
#define DMACCONFIG(d)       (*((uint32_t volatile*)(0x38200030 + 0x1700000 * (d))))
#define DMACSYNC(d)         (*((uint32_t volatile*)(0x38200034 + 0x1700000 * (d))))
#define DMACCLLI(d, c)      (*((struct dma_lli volatile*)(0x38200100 + 0x1700000 * (d) + 0x20 * (c))))
#define DMACCSRCADDR(d, c)  (*((const void* volatile*)(0x38200100 + 0x1700000 * (d) + 0x20 * (c))))
#define DMACCDESTADDR(d, c) (*((void* volatile*)(0x38200104 + 0x1700000 * (d) + 0x20 * (c))))
#define DMACCNEXTLLI(d, c)  (*((const void* volatile*)(0x38200108 + 0x1700000 * (d) + 0x20 * (c))))
#define DMACCCONTROL(d, c)  (*((uint32_t volatile*)(0x3820010c + 0x1700000 * (d) + 0x20 * (c))))
#define DMACCCONFIG(d, c)   (*((uint32_t volatile*)(0x38200110 + 0x1700000 * (d) + 0x20 * (c))))
#define DMAC0INTSTS         (*((uint32_t volatile*)(0x38200000)))
#define DMAC0INTTCSTS       (*((uint32_t volatile*)(0x38200004)))
#define DMAC0INTTCCLR       (*((uint32_t volatile*)(0x38200008)))
#define DMAC0INTERRSTS      (*((uint32_t volatile*)(0x3820000c)))
#define DMAC0INTERRCLR      (*((uint32_t volatile*)(0x38200010)))
#define DMAC0RAWINTTCSTS    (*((uint32_t volatile*)(0x38200014)))
#define DMAC0RAWINTERRSTS   (*((uint32_t volatile*)(0x38200018)))
#define DMAC0ENABLEDCHANS   (*((uint32_t volatile*)(0x3820001c)))
#define DMAC0SOFTBREQ       (*((uint32_t volatile*)(0x38200020)))
#define DMAC0SOFTSREQ       (*((uint32_t volatile*)(0x38200024)))
#define DMAC0SOFTLBREQ      (*((uint32_t volatile*)(0x38200028)))
#define DMAC0SOFTLSREQ      (*((uint32_t volatile*)(0x3820002c)))
#define DMAC0CONFIG         (*((uint32_t volatile*)(0x38200030)))
#define DMAC0SYNC           (*((uint32_t volatile*)(0x38200034)))
#define DMAC0CLLI(c)        (*((struct dma_lli volatile*)(0x38200100 + 0x20 * (c))))
#define DMAC0CSRCADDR(c)    (*((const void* volatile*)(0x38200100 + 0x20 * (c))))
#define DMAC0CDESTADDR(c)   (*((void* volatile*)(0x38200104 + 0x20 * (c))))
#define DMAC0CNEXTLLI(c)    (*((const void* volatile*)(0x38200108 + 0x20 * (c))))
#define DMAC0CCONTROL(c)    (*((uint32_t volatile*)(0x3820010c + 0x20 * (c))))
#define DMAC0CCONFIG(c)     (*((uint32_t volatile*)(0x38200110 + 0x20 * (c))))
#define DMAC0C0LLI          (*((struct dma_lli volatile*)(0x38200100)))
#define DMAC0C0SRCADDR      (*((const void* volatile*)(0x38200100)))
#define DMAC0C0DESTADDR     (*((void* volatile*)(0x38200104)))
#define DMAC0C0NEXTLLI      (*((const struct dma_lli* volatile*)(0x38200108)))
#define DMAC0C0CONTROL      (*((uint32_t volatile*)(0x3820010c)))
#define DMAC0C0CONFIG       (*((uint32_t volatile*)(0x38200110)))
#define DMAC0C1LLI          (*((struct dma_lli volatile*)(0x38200120)))
#define DMAC0C1SRCADDR      (*((const void* volatile*)(0x38200120)))
#define DMAC0C1DESTADDR     (*((void* volatile*)(0x38200124)))
#define DMAC0C1NEXTLLI      (*((const struct dma_lli* volatile*)(0x38200128)))
#define DMAC0C1CONTROL      (*((uint32_t volatile*)(0x3820012c)))
#define DMAC0C1CONFIG       (*((uint32_t volatile*)(0x38200130)))
#define DMAC0C2LLI          (*((struct dma_lli volatile*)(0x38200140)))
#define DMAC0C2SRCADDR      (*((const void* volatile*)(0x38200140)))
#define DMAC0C2DESTADDR     (*((void* volatile*)(0x38200144)))
#define DMAC0C2NEXTLLI      (*((const struct dma_lli* volatile*)(0x38200148)))
#define DMAC0C2CONTROL      (*((uint32_t volatile*)(0x3820014c)))
#define DMAC0C2CONFIG       (*((uint32_t volatile*)(0x38200150)))
#define DMAC0C3LLI          (*((struct dma_lli volatile*)(0x38200160)))
#define DMAC0C3SRCADDR      (*((const void* volatile*)(0x38200160)))
#define DMAC0C3DESTADDR     (*((void* volatile*)(0x38200164)))
#define DMAC0C3NEXTLLI      (*((const struct dma_lli* volatile*)(0x38200168)))
#define DMAC0C3CONTROL      (*((uint32_t volatile*)(0x3820016c)))
#define DMAC0C3CONFIG       (*((uint32_t volatile*)(0x38200170)))
#define DMAC0C4LLI          (*((struct dma_lli volatile*)(0x38200180)))
#define DMAC0C4SRCADDR      (*((const void* volatile*)(0x38200180)))
#define DMAC0C4DESTADDR     (*((void* volatile*)(0x38200184)))
#define DMAC0C4NEXTLLI      (*((const struct dma_lli* volatile*)(0x38200188)))
#define DMAC0C4CONTROL      (*((uint32_t volatile*)(0x3820018c)))
#define DMAC0C4CONFIG       (*((uint32_t volatile*)(0x38200190)))
#define DMAC0C5LLI          (*((struct dma_lli volatile*)(0x382001a0)))
#define DMAC0C5SRCADDR      (*((const void* volatile*)(0x382001a0)))
#define DMAC0C5DESTADDR     (*((void* volatile*)(0x382001a4)))
#define DMAC0C5NEXTLLI      (*((const struct dma_lli* volatile*)(0x382001a8)))
#define DMAC0C5CONTROL      (*((uint32_t volatile*)(0x382001ac)))
#define DMAC0C5CONFIG       (*((uint32_t volatile*)(0x382001b0)))
#define DMAC0C6LLI          (*((struct dma_lli volatile*)(0x382001c0)))
#define DMAC0C6SRCADDR      (*((const void* volatile*)(0x382001c0)))
#define DMAC0C6DESTADDR     (*((void* volatile*)(0x382001c4)))
#define DMAC0C6NEXTLLI      (*((const struct dma_lli* volatile*)(0x382001c8)))
#define DMAC0C6CONTROL      (*((uint32_t volatile*)(0x382001cc)))
#define DMAC0C6CONFIG       (*((uint32_t volatile*)(0x382001d0)))
#define DMAC0C7LLI          (*((struct dma_lli volatile*)(0x382001e0)))
#define DMAC0C7SRCADDR      (*((const void* volatile*)(0x382001e0)))
#define DMAC0C7DESTADDR     (*((void* volatile*)(0x382001e4)))
#define DMAC0C7NEXTLLI      (*((const struct dma_lli* volatile*)(0x382001e8)))
#define DMAC0C7CONTROL      (*((uint32_t volatile*)(0x382001ec)))
#define DMAC0C7CONFIG       (*((uint32_t volatile*)(0x382001f0)))
#define DMAC1INTSTS         (*((uint32_t volatile*)(0x39900000)))
#define DMAC1INTTCSTS       (*((uint32_t volatile*)(0x39900004)))
#define DMAC1INTTCCLR       (*((uint32_t volatile*)(0x39900008)))
#define DMAC1INTERRSTS      (*((uint32_t volatile*)(0x3990000c)))
#define DMAC1INTERRCLR      (*((uint32_t volatile*)(0x39900010)))
#define DMAC1RAWINTTCSTS    (*((uint32_t volatile*)(0x39900014)))
#define DMAC1RAWINTERRSTS   (*((uint32_t volatile*)(0x39900018)))
#define DMAC1ENABLEDCHANS   (*((uint32_t volatile*)(0x3990001c)))
#define DMAC1SOFTBREQ       (*((uint32_t volatile*)(0x39900020)))
#define DMAC1SOFTSREQ       (*((uint32_t volatile*)(0x39900024)))
#define DMAC1SOFTLBREQ      (*((uint32_t volatile*)(0x39900028)))
#define DMAC1SOFTLSREQ      (*((uint32_t volatile*)(0x3990002c)))
#define DMAC1CONFIG         (*((uint32_t volatile*)(0x39900030)))
#define DMAC1SYNC           (*((uint32_t volatile*)(0x39900034)))
#define DMAC1CLLI(c)        (*((struct dma_lli volatile*)(0x39900100 + 0x20 * (c))))
#define DMAC1CSRCADDR(c)    (*((const void* volatile*)(0x39900100 + 0x20 * (c))))
#define DMAC1CDESTADDR(c)   (*((void* volatile*)(0x39900104 + 0x20 * (c))))
#define DMAC1CNEXTLLI(c)    (*((const void* volatile*)(0x39900108 + 0x20 * (c))))
#define DMAC1CCONTROL(c)    (*((uint32_t volatile*)(0x3990010c + 0x20 * (c))))
#define DMAC1CCONFIG(c)     (*((uint32_t volatile*)(0x39900110 + 0x20 * (c))))
#define DMAC1C0LLI          (*((struct dma_lli volatile*)(0x39900100)))
#define DMAC1C0SRCADDR      (*((const void* volatile*)(0x39900100)))
#define DMAC1C0DESTADDR     (*((void* volatile*)(0x39900104)))
#define DMAC1C0NEXTLLI      (*((const struct dma_lli* volatile*)(0x39900108)))
#define DMAC1C0CONTROL      (*((uint32_t volatile*)(0x3990010c)))
#define DMAC1C0CONFIG       (*((uint32_t volatile*)(0x39900110)))
#define DMAC1C1LLI          (*((struct dma_lli volatile*)(0x39900120)))
#define DMAC1C1SRCADDR      (*((const void* volatile*)(0x39900120)))
#define DMAC1C1DESTADDR     (*((void* volatile*)(0x39900124)))
#define DMAC1C1NEXTLLI      (*((const struct dma_lli* volatile*)(0x39900128)))
#define DMAC1C1CONTROL      (*((uint32_t volatile*)(0x3990012c)))
#define DMAC1C1CONFIG       (*((uint32_t volatile*)(0x39900130)))
#define DMAC1C2LLI          (*((struct dma_lli volatile*)(0x39900140)))
#define DMAC1C2SRCADDR      (*((const void* volatile*)(0x39900140)))
#define DMAC1C2DESTADDR     (*((void* volatile*)(0x39900144)))
#define DMAC1C2NEXTLLI      (*((const struct dma_lli* volatile*)(0x39900148)))
#define DMAC1C2CONTROL      (*((uint32_t volatile*)(0x3990014c)))
#define DMAC1C2CONFIG       (*((uint32_t volatile*)(0x39900150)))
#define DMAC1C3LLI          (*((struct dma_lli volatile*)(0x39900160)))
#define DMAC1C3SRCADDR      (*((const void* volatile*)(0x39900160)))
#define DMAC1C3DESTADDR     (*((void* volatile*)(0x39900164)))
#define DMAC1C3NEXTLLI      (*((volatile void**)(0x39900168)))
#define DMAC1C3CONTROL      (*((uint32_t volatile*)(0x3990016c)))
#define DMAC1C3CONFIG       (*((uint32_t volatile*)(0x39900170)))
#define DMAC1C4LLI          (*((struct dma_lli volatile*)(0x39900180)))
#define DMAC1C4SRCADDR      (*((const void* volatile*)(0x39900180)))
#define DMAC1C4DESTADDR     (*((void* volatile*)(0x39900184)))
#define DMAC1C4NEXTLLI      (*((const struct dma_lli* volatile*)(0x39900188)))
#define DMAC1C4CONTROL      (*((uint32_t volatile*)(0x3990018c)))
#define DMAC1C4CONFIG       (*((uint32_t volatile*)(0x39900190)))
#define DMAC1C5LLI          (*((struct dma_lli volatile*)(0x399001a0)))
#define DMAC1C5SRCADDR      (*((const void* volatile*)(0x399001a0)))
#define DMAC1C5DESTADDR     (*((void* volatile*)(0x399001a4)))
#define DMAC1C5NEXTLLI      (*((const struct dma_lli* volatile*)(0x399001a8)))
#define DMAC1C5CONTROL      (*((uint32_t volatile*)(0x399001ac)))
#define DMAC1C5CONFIG       (*((uint32_t volatile*)(0x399001b0)))
#define DMAC1C6LLI          (*((struct dma_lli volatile*)(0x399001c0)))
#define DMAC1C6SRCADDR      (*((const void* volatile*)(0x399001c0)))
#define DMAC1C6DESTADDR     (*((void* volatile*)(0x399001c4)))
#define DMAC1C6NEXTLLI      (*((const struct dma_lli* volatile*)(0x399001c8)))
#define DMAC1C6CONTROL      (*((uint32_t volatile*)(0x399001cc)))
#define DMAC1C6CONFIG       (*((uint32_t volatile*)(0x399001d0)))
#define DMAC1C7LLI          (*((struct dma_lli volatile*)(0x399001e0)))
#define DMAC1C7SRCADDR      (*((const void* volatile*)(0x399001e0)))
#define DMAC1C7DESTADDR     (*((void* volatile*)(0x399001e4)))
#define DMAC1C7NEXTLLI      (*((const struct dma_lli* volatile*)(0x399001e8)))
#define DMAC1C7CONTROL      (*((uint32_t volatile*)(0x399001ec)))
#define DMAC1C7CONFIG       (*((uint32_t volatile*)(0x399001f0)))


/////LCD/////
#define LCDWCMD   (*((uint32_t volatile*)(0x38300004)))
#define LCDSTATUS (*((uint32_t volatile*)(0x3830001c)))
#define LCDWDATA  (*((uint32_t volatile*)(0x38300040)))


/////CLOCK GATES/////
#define CLOCKGATE_USB_1 2
#define CLOCKGATE_USB_2 35


/////INTERRUPTS/////
#define IRQ_TIMER 8
#define IRQ_USB_FUNC 19
#define IRQ_DMAC(d) 16 + d
#define IRQ_DMAC0 16
#define IRQ_DMAC1 17


#endif
