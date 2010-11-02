//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __S5L8720_H__
#define __S5L8720_H__

#include "global.h"


/////SYSCON/////
#define PWRCON(i)    (*((volatile uint32_t*)(0x3C500000 \
                                           + ((i) == 4 ? 0x6C : \
                                             ((i) == 3 ? 0x68 : \
                                             ((i) == 2 ? 0x58 : \
                                             ((i) == 1 ? 0x4C : \
                                                         0x48)))))))


/////TIMER/////
#define TACON        (*((volatile uint32_t*)(0x3C700000)))
#define TACMD        (*((volatile uint32_t*)(0x3C700004)))
#define TADATA0      (*((volatile uint32_t*)(0x3C700008)))
#define TADATA1      (*((volatile uint32_t*)(0x3C70000C)))
#define TAPRE        (*((volatile uint32_t*)(0x3C700010)))
#define TACNT        (*((volatile uint32_t*)(0x3C700014)))
#define TBCON        (*((volatile uint32_t*)(0x3C700020)))
#define TBCMD        (*((volatile uint32_t*)(0x3C700024)))
#define TBDATA0      (*((volatile uint32_t*)(0x3C700028)))
#define TBDATA1      (*((volatile uint32_t*)(0x3C70002C)))
#define TBPRE        (*((volatile uint32_t*)(0x3C700030)))
#define TBCNT        (*((volatile uint32_t*)(0x3C700034)))
#define TCCON        (*((volatile uint32_t*)(0x3C700040)))
#define TCCMD        (*((volatile uint32_t*)(0x3C700044)))
#define TCDATA0      (*((volatile uint32_t*)(0x3C700048)))
#define TCDATA1      (*((volatile uint32_t*)(0x3C70004C)))
#define TCPRE        (*((volatile uint32_t*)(0x3C700050)))
#define TCCNT        (*((volatile uint32_t*)(0x3C700054)))
#define TDCON        (*((volatile uint32_t*)(0x3C700060)))
#define TDCMD        (*((volatile uint32_t*)(0x3C700064)))
#define TDDATA0      (*((volatile uint32_t*)(0x3C700068)))
#define TDDATA1      (*((volatile uint32_t*)(0x3C70006C)))
#define TDPRE        (*((volatile uint32_t*)(0x3C700070)))
#define TDCNT        (*((volatile uint32_t*)(0x3C700074)))
#define TECON        (*((volatile uint32_t*)(0x3C7000A0)))
#define TECMD        (*((volatile uint32_t*)(0x3C7000A4)))
#define TEDATA0      (*((volatile uint32_t*)(0x3C7000A8)))
#define TEDATA1      (*((volatile uint32_t*)(0x3C7000AC)))
#define TEPRE        (*((volatile uint32_t*)(0x3C7000B0)))
#define TECNT        (*((volatile uint32_t*)(0x3C7000B4)))
#define TFCON        (*((volatile uint32_t*)(0x3C7000C0)))
#define TFCMD        (*((volatile uint32_t*)(0x3C7000C4)))
#define TFDATA0      (*((volatile uint32_t*)(0x3C7000C8)))
#define TFDATA1      (*((volatile uint32_t*)(0x3C7000CC)))
#define TFPRE        (*((volatile uint32_t*)(0x3C7000D0)))
#define TFCNT        (*((volatile uint32_t*)(0x3C7000D4)))
#define TGCON        (*((volatile uint32_t*)(0x3C7000E0)))
#define TGCMD        (*((volatile uint32_t*)(0x3C7000E4)))
#define TGDATA0      (*((volatile uint32_t*)(0x3C7000E8)))
#define TGDATA1      (*((volatile uint32_t*)(0x3C7000EC)))
#define TGPRE        (*((volatile uint32_t*)(0x3C7000F0)))
#define TGCNT        (*((volatile uint32_t*)(0x3C7000F4)))
#define THCON        (*((volatile uint32_t*)(0x3C700100)))
#define THCMD        (*((volatile uint32_t*)(0x3C700104)))
#define THDATA0      (*((volatile uint32_t*)(0x3C700108)))
#define THDATA1      (*((volatile uint32_t*)(0x3C70010C)))
#define THPRE        (*((volatile uint32_t*)(0x3C700110)))
#define THCNT        (*((volatile uint32_t*)(0x3C700114)))


/////USB/////
#define OTGBASE 0x38400000
#define PHYBASE 0x3C400000


/////I2C/////
#define IICCON(bus)  (*((volatile uint32_t*)(0x3C600000 + 0x300000 * (bus))))
#define IICSTAT(bus) (*((volatile uint32_t*)(0x3C600004 + 0x300000 * (bus))))
#define IICADD(bus)  (*((volatile uint32_t*)(0x3C600008 + 0x300000 * (bus))))
#define IICDS(bus)   (*((volatile uint32_t*)(0x3C60000C + 0x300000 * (bus))))


/////INTERRUPTS/////
#define VICIRQSTATUS(v)       (*((volatile uint32_t*)(0x38E00000 + 0x1000 * (v))))
#define VICFIQSTATUS(v)       (*((volatile uint32_t*)(0x38E00004 + 0x1000 * (v))))
#define VICRAWINTR(v)         (*((volatile uint32_t*)(0x38E00008 + 0x1000 * (v))))
#define VICINTSELECT(v)       (*((volatile uint32_t*)(0x38E0000C + 0x1000 * (v))))
#define VICINTENABLE(v)       (*((volatile uint32_t*)(0x38E00010 + 0x1000 * (v))))
#define VICINTENCLEAR(v)      (*((volatile uint32_t*)(0x38E00014 + 0x1000 * (v))))
#define VICSOFTINT(v)         (*((volatile uint32_t*)(0x38E00018 + 0x1000 * (v))))
#define VICSOFTINTCLEAR(v)    (*((volatile uint32_t*)(0x38E0001C + 0x1000 * (v))))
#define VICPROTECTION(v)      (*((volatile uint32_t*)(0x38E00020 + 0x1000 * (v))))
#define VICSWPRIORITYMASK(v)  (*((volatile uint32_t*)(0x38E00024 + 0x1000 * (v))))
#define VICPRIORITYDAISY(v)   (*((volatile uint32_t*)(0x38E00028 + 0x1000 * (v))))
#define VICVECTADDR(v, i)     (*((volatile uint32_t*)(0x38E00100 + 0x1000 * (v) + 4 * (i))))
#define VICVECTPRIORITY(v, i) (*((volatile uint32_t*)(0x38E00200 + 0x1000 * (v) + 4 * (i))))
#define VICADDRESS(v)         (*((volatile uint32_t*)(0x38E00F00 + 0x1000 * (v))))
#define VIC0IRQSTATUS         (*((volatile uint32_t*)(0x38E00000)))
#define VIC0FIQSTATUS         (*((volatile uint32_t*)(0x38E00004)))
#define VIC0RAWINTR           (*((volatile uint32_t*)(0x38E00008)))
#define VIC0INTSELECT         (*((volatile uint32_t*)(0x38E0000C)))
#define VIC0INTENABLE         (*((volatile uint32_t*)(0x38E00010)))
#define VIC0INTENCLEAR        (*((volatile uint32_t*)(0x38E00014)))
#define VIC0SOFTINT           (*((volatile uint32_t*)(0x38E00018)))
#define VIC0SOFTINTCLEAR      (*((volatile uint32_t*)(0x38E0001C)))
#define VIC0PROTECTION        (*((volatile uint32_t*)(0x38E00020)))
#define VIC0SWPRIORITYMASK    (*((volatile uint32_t*)(0x38E00024)))
#define VIC0PRIORITYDAISY     (*((volatile uint32_t*)(0x38E00028)))
#define VIC0VECTADDR(i)       (*((volatile uint32_t*)(0x38E00100 + 4 * (i))))
#define VIC0VECTADDR0         (*((volatile uint32_t*)(0x38E00100)))
#define VIC0VECTADDR1         (*((volatile uint32_t*)(0x38E00104)))
#define VIC0VECTADDR2         (*((volatile uint32_t*)(0x38E00108)))
#define VIC0VECTADDR3         (*((volatile uint32_t*)(0x38E0010C)))
#define VIC0VECTADDR4         (*((volatile uint32_t*)(0x38E00110)))
#define VIC0VECTADDR5         (*((volatile uint32_t*)(0x38E00114)))
#define VIC0VECTADDR6         (*((volatile uint32_t*)(0x38E00118)))
#define VIC0VECTADDR7         (*((volatile uint32_t*)(0x38E0011C)))
#define VIC0VECTADDR8         (*((volatile uint32_t*)(0x38E00120)))
#define VIC0VECTADDR9         (*((volatile uint32_t*)(0x38E00124)))
#define VIC0VECTADDR10        (*((volatile uint32_t*)(0x38E00128)))
#define VIC0VECTADDR11        (*((volatile uint32_t*)(0x38E0012C)))
#define VIC0VECTADDR12        (*((volatile uint32_t*)(0x38E00130)))
#define VIC0VECTADDR13        (*((volatile uint32_t*)(0x38E00134)))
#define VIC0VECTADDR14        (*((volatile uint32_t*)(0x38E00138)))
#define VIC0VECTADDR15        (*((volatile uint32_t*)(0x38E0013C)))
#define VIC0VECTADDR16        (*((volatile uint32_t*)(0x38E00140)))
#define VIC0VECTADDR17        (*((volatile uint32_t*)(0x38E00144)))
#define VIC0VECTADDR18        (*((volatile uint32_t*)(0x38E00148)))
#define VIC0VECTADDR19        (*((volatile uint32_t*)(0x38E0014C)))
#define VIC0VECTADDR20        (*((volatile uint32_t*)(0x38E00150)))
#define VIC0VECTADDR21        (*((volatile uint32_t*)(0x38E00154)))
#define VIC0VECTADDR22        (*((volatile uint32_t*)(0x38E00158)))
#define VIC0VECTADDR23        (*((volatile uint32_t*)(0x38E0015C)))
#define VIC0VECTADDR24        (*((volatile uint32_t*)(0x38E00160)))
#define VIC0VECTADDR25        (*((volatile uint32_t*)(0x38E00164)))
#define VIC0VECTADDR26        (*((volatile uint32_t*)(0x38E00168)))
#define VIC0VECTADDR27        (*((volatile uint32_t*)(0x38E0016C)))
#define VIC0VECTADDR28        (*((volatile uint32_t*)(0x38E00170)))
#define VIC0VECTADDR29        (*((volatile uint32_t*)(0x38E00174)))
#define VIC0VECTADDR30        (*((volatile uint32_t*)(0x38E00178)))
#define VIC0VECTADDR31        (*((volatile uint32_t*)(0x38E0017C)))
#define VIC0VECTPRIORITY(i)   (*((volatile uint32_t*)(0x38E00200 + 4 * (i))))
#define VIC0VECTPRIORITY0     (*((volatile uint32_t*)(0x38E00200)))
#define VIC0VECTPRIORITY1     (*((volatile uint32_t*)(0x38E00204)))
#define VIC0VECTPRIORITY2     (*((volatile uint32_t*)(0x38E00208)))
#define VIC0VECTPRIORITY3     (*((volatile uint32_t*)(0x38E0020C)))
#define VIC0VECTPRIORITY4     (*((volatile uint32_t*)(0x38E00210)))
#define VIC0VECTPRIORITY5     (*((volatile uint32_t*)(0x38E00214)))
#define VIC0VECTPRIORITY6     (*((volatile uint32_t*)(0x38E00218)))
#define VIC0VECTPRIORITY7     (*((volatile uint32_t*)(0x38E0021C)))
#define VIC0VECTPRIORITY8     (*((volatile uint32_t*)(0x38E00220)))
#define VIC0VECTPRIORITY9     (*((volatile uint32_t*)(0x38E00224)))
#define VIC0VECTPRIORITY10    (*((volatile uint32_t*)(0x38E00228)))
#define VIC0VECTPRIORITY11    (*((volatile uint32_t*)(0x38E0022C)))
#define VIC0VECTPRIORITY12    (*((volatile uint32_t*)(0x38E00230)))
#define VIC0VECTPRIORITY13    (*((volatile uint32_t*)(0x38E00234)))
#define VIC0VECTPRIORITY14    (*((volatile uint32_t*)(0x38E00238)))
#define VIC0VECTPRIORITY15    (*((volatile uint32_t*)(0x38E0023C)))
#define VIC0VECTPRIORITY16    (*((volatile uint32_t*)(0x38E00240)))
#define VIC0VECTPRIORITY17    (*((volatile uint32_t*)(0x38E00244)))
#define VIC0VECTPRIORITY18    (*((volatile uint32_t*)(0x38E00248)))
#define VIC0VECTPRIORITY19    (*((volatile uint32_t*)(0x38E0024C)))
#define VIC0VECTPRIORITY20    (*((volatile uint32_t*)(0x38E00250)))
#define VIC0VECTPRIORITY21    (*((volatile uint32_t*)(0x38E00254)))
#define VIC0VECTPRIORITY22    (*((volatile uint32_t*)(0x38E00258)))
#define VIC0VECTPRIORITY23    (*((volatile uint32_t*)(0x38E0025C)))
#define VIC0VECTPRIORITY24    (*((volatile uint32_t*)(0x38E00260)))
#define VIC0VECTPRIORITY25    (*((volatile uint32_t*)(0x38E00264)))
#define VIC0VECTPRIORITY26    (*((volatile uint32_t*)(0x38E00268)))
#define VIC0VECTPRIORITY27    (*((volatile uint32_t*)(0x38E0026C)))
#define VIC0VECTPRIORITY28    (*((volatile uint32_t*)(0x38E00270)))
#define VIC0VECTPRIORITY29    (*((volatile uint32_t*)(0x38E00274)))
#define VIC0VECTPRIORITY30    (*((volatile uint32_t*)(0x38E00278)))
#define VIC0VECTPRIORITY31    (*((volatile uint32_t*)(0x38E0027C)))
#define VIC0ADDRESS           (*((volatile uint32_t*)(0x38E00F00)))
#define VIC1IRQSTATUS         (*((volatile uint32_t*)(0x38E01000)))
#define VIC1FIQSTATUS         (*((volatile uint32_t*)(0x38E01004)))
#define VIC1RAWINTR           (*((volatile uint32_t*)(0x38E01008)))
#define VIC1INTSELECT         (*((volatile uint32_t*)(0x38E0100C)))
#define VIC1INTENABLE         (*((volatile uint32_t*)(0x38E01010)))
#define VIC1INTENCLEAR        (*((volatile uint32_t*)(0x38E01014)))
#define VIC1SOFTINT           (*((volatile uint32_t*)(0x38E01018)))
#define VIC1SOFTINTCLEAR      (*((volatile uint32_t*)(0x38E0101C)))
#define VIC1PROTECTION        (*((volatile uint32_t*)(0x38E01020)))
#define VIC1SWPRIORITYMASK    (*((volatile uint32_t*)(0x38E01024)))
#define VIC1PRIORITYDAISY     (*((volatile uint32_t*)(0x38E01028)))
#define VIC1VECTADDR(i)       (*((volatile uint32_t*)(0x38E01100 + 4 * (i))))
#define VIC1VECTADDR0         (*((volatile uint32_t*)(0x38E01100)))
#define VIC1VECTADDR1         (*((volatile uint32_t*)(0x38E01104)))
#define VIC1VECTADDR2         (*((volatile uint32_t*)(0x38E01108)))
#define VIC1VECTADDR3         (*((volatile uint32_t*)(0x38E0110C)))
#define VIC1VECTADDR4         (*((volatile uint32_t*)(0x38E01110)))
#define VIC1VECTADDR5         (*((volatile uint32_t*)(0x38E01114)))
#define VIC1VECTADDR6         (*((volatile uint32_t*)(0x38E01118)))
#define VIC1VECTADDR7         (*((volatile uint32_t*)(0x38E0111C)))
#define VIC1VECTADDR8         (*((volatile uint32_t*)(0x38E01120)))
#define VIC1VECTADDR9         (*((volatile uint32_t*)(0x38E01124)))
#define VIC1VECTADDR10        (*((volatile uint32_t*)(0x38E01128)))
#define VIC1VECTADDR11        (*((volatile uint32_t*)(0x38E0112C)))
#define VIC1VECTADDR12        (*((volatile uint32_t*)(0x38E01130)))
#define VIC1VECTADDR13        (*((volatile uint32_t*)(0x38E01134)))
#define VIC1VECTADDR14        (*((volatile uint32_t*)(0x38E01138)))
#define VIC1VECTADDR15        (*((volatile uint32_t*)(0x38E0113C)))
#define VIC1VECTADDR16        (*((volatile uint32_t*)(0x38E01140)))
#define VIC1VECTADDR17        (*((volatile uint32_t*)(0x38E01144)))
#define VIC1VECTADDR18        (*((volatile uint32_t*)(0x38E01148)))
#define VIC1VECTADDR19        (*((volatile uint32_t*)(0x38E0114C)))
#define VIC1VECTADDR20        (*((volatile uint32_t*)(0x38E01150)))
#define VIC1VECTADDR21        (*((volatile uint32_t*)(0x38E01154)))
#define VIC1VECTADDR22        (*((volatile uint32_t*)(0x38E01158)))
#define VIC1VECTADDR23        (*((volatile uint32_t*)(0x38E0115C)))
#define VIC1VECTADDR24        (*((volatile uint32_t*)(0x38E01160)))
#define VIC1VECTADDR25        (*((volatile uint32_t*)(0x38E01164)))
#define VIC1VECTADDR26        (*((volatile uint32_t*)(0x38E01168)))
#define VIC1VECTADDR27        (*((volatile uint32_t*)(0x38E0116C)))
#define VIC1VECTADDR28        (*((volatile uint32_t*)(0x38E01170)))
#define VIC1VECTADDR29        (*((volatile uint32_t*)(0x38E01174)))
#define VIC1VECTADDR30        (*((volatile uint32_t*)(0x38E01178)))
#define VIC1VECTADDR31        (*((volatile uint32_t*)(0x38E0117C)))
#define VIC1VECTPRIORITY(i)   (*((volatile uint32_t*)(0x38E01200 + 4 * (i))))
#define VIC1VECTPRIORITY0     (*((volatile uint32_t*)(0x38E01200)))
#define VIC1VECTPRIORITY1     (*((volatile uint32_t*)(0x38E01204)))
#define VIC1VECTPRIORITY2     (*((volatile uint32_t*)(0x38E01208)))
#define VIC1VECTPRIORITY3     (*((volatile uint32_t*)(0x38E0120C)))
#define VIC1VECTPRIORITY4     (*((volatile uint32_t*)(0x38E01210)))
#define VIC1VECTPRIORITY5     (*((volatile uint32_t*)(0x38E01214)))
#define VIC1VECTPRIORITY6     (*((volatile uint32_t*)(0x38E01218)))
#define VIC1VECTPRIORITY7     (*((volatile uint32_t*)(0x38E0121C)))
#define VIC1VECTPRIORITY8     (*((volatile uint32_t*)(0x38E01220)))
#define VIC1VECTPRIORITY9     (*((volatile uint32_t*)(0x38E01224)))
#define VIC1VECTPRIORITY10    (*((volatile uint32_t*)(0x38E01228)))
#define VIC1VECTPRIORITY11    (*((volatile uint32_t*)(0x38E0122C)))
#define VIC1VECTPRIORITY12    (*((volatile uint32_t*)(0x38E01230)))
#define VIC1VECTPRIORITY13    (*((volatile uint32_t*)(0x38E01234)))
#define VIC1VECTPRIORITY14    (*((volatile uint32_t*)(0x38E01238)))
#define VIC1VECTPRIORITY15    (*((volatile uint32_t*)(0x38E0123C)))
#define VIC1VECTPRIORITY16    (*((volatile uint32_t*)(0x38E01240)))
#define VIC1VECTPRIORITY17    (*((volatile uint32_t*)(0x38E01244)))
#define VIC1VECTPRIORITY18    (*((volatile uint32_t*)(0x38E01248)))
#define VIC1VECTPRIORITY19    (*((volatile uint32_t*)(0x38E0124C)))
#define VIC1VECTPRIORITY20    (*((volatile uint32_t*)(0x38E01250)))
#define VIC1VECTPRIORITY21    (*((volatile uint32_t*)(0x38E01254)))
#define VIC1VECTPRIORITY22    (*((volatile uint32_t*)(0x38E01258)))
#define VIC1VECTPRIORITY23    (*((volatile uint32_t*)(0x38E0125C)))
#define VIC1VECTPRIORITY24    (*((volatile uint32_t*)(0x38E01260)))
#define VIC1VECTPRIORITY25    (*((volatile uint32_t*)(0x38E01264)))
#define VIC1VECTPRIORITY26    (*((volatile uint32_t*)(0x38E01268)))
#define VIC1VECTPRIORITY27    (*((volatile uint32_t*)(0x38E0126C)))
#define VIC1VECTPRIORITY28    (*((volatile uint32_t*)(0x38E01270)))
#define VIC1VECTPRIORITY29    (*((volatile uint32_t*)(0x38E01274)))
#define VIC1VECTPRIORITY30    (*((volatile uint32_t*)(0x38E01278)))
#define VIC1VECTPRIORITY31    (*((volatile uint32_t*)(0x38E0127C)))
#define VIC1ADDRESS           (*((volatile uint32_t*)(0x38E01F00)))


/////CLOCK GATES/////
#define CLOCKGATE_USB_1 2
#define CLOCKGATE_USB_2 35


/////INTERRUPTS/////
#define IRQ_TIMER 8
#define IRQ_USB_FUNC 19


#endif
