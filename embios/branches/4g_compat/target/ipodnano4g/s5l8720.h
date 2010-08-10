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


#ifndef __S5L8701_H__
#define __S5L8701_H__

#include "global.h"


/////CLKCON/////
#define CLKCON       (*((volatile uint32_t*)(0x3C500000)))
#define PLL0PMS      (*((volatile uint32_t*)(0x3C500004)))
#define PLL1PMS      (*((volatile uint32_t*)(0x3C500008)))
#define PLL2PMS      (*((volatile uint32_t*)(0x3C50000C)))
#define PLL0LCNT     (*((volatile uint32_t*)(0x3C500014)))
#define PLL1LCNT     (*((volatile uint32_t*)(0x3C500018)))
#define PLL2LCNT     (*((volatile uint32_t*)(0x3C50001C)))
#define PLLLOCK      (*((volatile uint32_t*)(0x3C500020)))
#define PLLCON       (*((volatile uint32_t*)(0x3C500024)))
#define PWRCON       (*((volatile uint32_t*)(0x3C500028)))
#define PWRMODE      (*((volatile uint32_t*)(0x3C50002C)))
#define SWRCON       (*((volatile uint32_t*)(0x3C500030)))
#define RSTSR        (*((volatile uint32_t*)(0x3C500034)))
#define DSPCLKMD     (*((volatile uint32_t*)(0x3C500038)))
#define CLKCON2      (*((volatile uint32_t*)(0x3C50003C)))
#define PWRCONEXT    (*((volatile uint32_t*)(0x3C500040)))


/////ICU/////
#define SRCPND       (*((volatile uint32_t*)(0x39C00000)))
#define INTMOD       (*((volatile uint32_t*)(0x39C00004)))
#define INTMSK       (*((volatile uint32_t*)(0x39C00008)))
#define INTPRIO      (*((volatile uint32_t*)(0x39C0000C)))
#define INTPND       (*((volatile uint32_t*)(0x39C00010)))
#define INTOFFSET    (*((volatile uint32_t*)(0x39C00014)))
#define EINTPOL      (*((volatile uint32_t*)(0x39C00018)))
#define EINTPEND     (*((volatile uint32_t*)(0x39C0001C)))
#define EINTMSK      (*((volatile uint32_t*)(0x39C00020)))


/////GPIO/////
#define PCON0        (*((volatile uint32_t*)(0x3CF00000)))
#define PDAT0        (*((volatile uint32_t*)(0x3CF00004)))
#define PCON1        (*((volatile uint32_t*)(0x3CF00010)))
#define PDAT1        (*((volatile uint32_t*)(0x3CF00014)))
#define PCON2        (*((volatile uint32_t*)(0x3CF00020)))
#define PDAT2        (*((volatile uint32_t*)(0x3CF00024)))
#define PCON3        (*((volatile uint32_t*)(0x3CF00030)))
#define PDAT3        (*((volatile uint32_t*)(0x3CF00034)))
#define PCON4        (*((volatile uint32_t*)(0x3CF00040)))
#define PDAT4        (*((volatile uint32_t*)(0x3CF00044)))
#define PCON5        (*((volatile uint32_t*)(0x3CF00050)))
#define PDAT5        (*((volatile uint32_t*)(0x3CF00054)))
#define PUNK5        (*((volatile uint32_t*)(0x3CF0005C)))
#define PCON6        (*((volatile uint32_t*)(0x3CF00060)))
#define PDAT6        (*((volatile uint32_t*)(0x3CF00064)))
#define PCON7        (*((volatile uint32_t*)(0x3CF00070)))
#define PDAT7        (*((volatile uint32_t*)(0x3CF00074)))
#define PCON10       (*((volatile uint32_t*)(0x3CF000A0)))
#define PDAT10       (*((volatile uint32_t*)(0x3CF000A4)))
#define PCON11       (*((volatile uint32_t*)(0x3CF000B0)))
#define PDAT11       (*((volatile uint32_t*)(0x3CF000B4)))
#define PCON15       (*((volatile uint32_t*)(0x3CF000F0)))
#define PUNK15       (*((volatile uint32_t*)(0x3CF000FC)))


/////IODMA/////
#define DMABASE0     (*((volatile uint32_t*)(0x38400000)))
#define DMACON0      (*((volatile uint32_t*)(0x38400004)))
#define DMATCNT0     (*((volatile uint32_t*)(0x38400008)))
#define DMACADDR0    (*((volatile uint32_t*)(0x3840000C)))
#define DMACTCNT0    (*((volatile uint32_t*)(0x38400010)))
#define DMACOM0      (*((volatile uint32_t*)(0x38400014)))
#define DMANOF0      (*((volatile uint32_t*)(0x38400018)))
#define DMABASE1     (*((volatile uint32_t*)(0x38400020)))
#define DMACON1      (*((volatile uint32_t*)(0x38400024)))
#define DMATCNT1     (*((volatile uint32_t*)(0x38400028)))
#define DMACADDR1    (*((volatile uint32_t*)(0x3840002C)))
#define DMACTCNT1    (*((volatile uint32_t*)(0x38400030)))
#define DMACOM1      (*((volatile uint32_t*)(0x38400034)))
#define DMABASE2     (*((volatile uint32_t*)(0x38400040)))
#define DMACON2      (*((volatile uint32_t*)(0x38400044)))
#define DMATCNT2     (*((volatile uint32_t*)(0x38400048)))
#define DMACADDR2    (*((volatile uint32_t*)(0x3840004C)))
#define DMACTCNT2    (*((volatile uint32_t*)(0x38400050)))
#define DMACOM2      (*((volatile uint32_t*)(0x38400054)))
#define DMABASE3     (*((volatile uint32_t*)(0x38400060)))
#define DMACON3      (*((volatile uint32_t*)(0x38400064)))
#define DMATCNT3     (*((volatile uint32_t*)(0x38400068)))
#define DMACADDR3    (*((volatile uint32_t*)(0x3840006C)))
#define DMACTCNT3    (*((volatile uint32_t*)(0x38400070)))
#define DMACOM3      (*((volatile uint32_t*)(0x38400074)))
#define DMABASE4     (*((volatile uint32_t*)(0x38400080)))
#define DMACON4      (*((volatile uint32_t*)(0x38400084)))
#define DMATCNT4     (*((volatile uint32_t*)(0x38400088)))
#define DMACADDR4    (*((volatile uint32_t*)(0x3840008C)))
#define DMACTCNT4    (*((volatile uint32_t*)(0x38400090)))
#define DMACOM4      (*((volatile uint32_t*)(0x38400094)))
#define DMABASE5     (*((volatile uint32_t*)(0x384000A0)))
#define DMACON5      (*((volatile uint32_t*)(0x384000A4)))
#define DMATCNT5     (*((volatile uint32_t*)(0x384000A8)))
#define DMACADDR5    (*((volatile uint32_t*)(0x384000AC)))
#define DMACTCNT5    (*((volatile uint32_t*)(0x384000B0)))
#define DMACOM5      (*((volatile uint32_t*)(0x384000B4)))
#define DMABASE6     (*((volatile uint32_t*)(0x384000C0)))
#define DMACON6      (*((volatile uint32_t*)(0x384000C4)))
#define DMATCNT6     (*((volatile uint32_t*)(0x384000C8)))
#define DMACADDR6    (*((volatile uint32_t*)(0x384000CC)))
#define DMACTCNT6    (*((volatile uint32_t*)(0x384000D0)))
#define DMACOM6      (*((volatile uint32_t*)(0x384000D4)))
#define DMABASE7     (*((volatile uint32_t*)(0x384000E0)))
#define DMACON7      (*((volatile uint32_t*)(0x384000E4)))
#define DMATCNT7     (*((volatile uint32_t*)(0x384000E8)))
#define DMACADDR7    (*((volatile uint32_t*)(0x384000EC)))
#define DMACTCNT7    (*((volatile uint32_t*)(0x384000F0)))
#define DMACOM7      (*((volatile uint32_t*)(0x384000F4)))
#define DMABASE8     (*((volatile uint32_t*)(0x38400100)))
#define DMACON8      (*((volatile uint32_t*)(0x38400104)))
#define DMATCNT8     (*((volatile uint32_t*)(0x38400108)))
#define DMACADDR8    (*((volatile uint32_t*)(0x3840010C)))
#define DMACTCNT8    (*((volatile uint32_t*)(0x38400110)))
#define DMACOM8      (*((volatile uint32_t*)(0x38400114)))
#define DMAALLST     (*((volatile uint32_t*)(0x38400180)))
#define DMAALLST2    (*((volatile uint32_t*)(0x38400184)))
#define DMACON_DEVICE_SHIFT    30
#define DMACON_DIRECTION_SHIFT 29
#define DMACON_DATA_SIZE_SHIFT 22
#define DMACON_BURST_LEN_SHIFT 19
#define DMACOM_START           4
#define DMACOM_CLEARBOTHDONE   7
#define DMAALLST_WCOM0         (1 << 0)
#define DMAALLST_HCOM0         (1 << 1)
#define DMAALLST_DMABUSY0      (1 << 2)
#define DMAALLST_HOLD_SKIP     (1 << 3)
#define DMAALLST_WCOM1         (1 << 4)
#define DMAALLST_HCOM1         (1 << 5)
#define DMAALLST_DMABUSY1      (1 << 6)
#define DMAALLST_WCOM2         (1 << 8)
#define DMAALLST_HCOM2         (1 << 9)
#define DMAALLST_DMABUSY2      (1 << 10)
#define DMAALLST_WCOM3         (1 << 12)
#define DMAALLST_HCOM3         (1 << 13)
#define DMAALLST_DMABUSY3      (1 << 14)
#define DMAALLST_CHAN0_MASK    (0xF << 0)
#define DMAALLST_CHAN1_MASK    (0xF << 4)
#define DMAALLST_CHAN2_MASK    (0xF << 8)
#define DMAALLST_CHAN3_MASK    (0xF << 12)


/////FMC/////
#define FMCTRL0      (*((volatile uint32_t*)(0x39400000)))
#define FMCTRL1      (*((volatile uint32_t*)(0x39400004)))
#define FMCMD        (*((volatile uint32_t*)(0x39400008)))
#define FMADDR0      (*((volatile uint32_t*)(0x3940000C)))
#define FMADDR1      (*((volatile uint32_t*)(0x39400010)))
#define FMANUM       (*((volatile uint32_t*)(0x3940002C)))
#define FMDNUM       (*((volatile uint32_t*)(0x39400030)))
#define FMCSTAT      (*((volatile uint32_t*)(0x39400048)))
#define FMFIFO       (*((volatile uint32_t*)(0x39400080)))
#define RS_ECC_CTRL  (*((volatile uint32_t*)(0x39400100)))
#define FMCTRL0_ENABLEDMA      (1 << 10)
#define FMCTRL0_UNK1           (1 << 11)
#define FMCTRL1_DOTRANSADDR    (1 << 0)
#define FMCTRL1_DOREADDATA     (1 << 1)
#define FMCTRL1_DOWRITEDATA    (1 << 2)
#define FMCTRL1_CLEARWFIFO     (1 << 6)
#define FMCTRL1_CLEARRFIFO     (1 << 7)
#define FMCSTAT_RBB            (1 << 0)
#define FMCSTAT_RBBDONE        (1 << 1)
#define FMCSTAT_CMDDONE        (1 << 2)
#define FMCSTAT_ADDRDONE       (1 << 3)
#define FMCSTAT_BANK0READY     (1 << 4)
#define FMCSTAT_BANK1READY     (1 << 5)
#define FMCSTAT_BANK2READY     (1 << 6)
#define FMCSTAT_BANK3READY     (1 << 7)


/////ECC/////
#define ECC_DATA_PTR  (*((volatile uint32_t*)(0x39E00004)))
#define ECC_SPARE_PTR (*((volatile uint32_t*)(0x39E00008)))
#define ECC_CTRL      (*((volatile uint32_t*)(0x39E0000C)))
#define ECC_RESULT    (*((volatile uint32_t*)(0x39E00010)))
#define ECC_UNK1      (*((volatile uint32_t*)(0x39E00014)))
#define ECC_INT_CLR   (*((volatile uint32_t*)(0x39E00040)))
#define ECCCTRL_STARTDECODING  (1 << 0)
#define ECCCTRL_STARTENCODING  (1 << 1)
#define ECCCTRL_STARTDECNOSYND (1 << 2)


/////CLICKWHEEL/////
#define WHEEL00      (*((volatile uint32_t*)(0x3C200000)))
#define WHEEL04      (*((volatile uint32_t*)(0x3C200004)))
#define WHEEL08      (*((volatile uint32_t*)(0x3C200008)))
#define WHEEL0C      (*((volatile uint32_t*)(0x3C20000C)))
#define WHEEL10      (*((volatile uint32_t*)(0x3C200010)))
#define WHEELINT     (*((volatile uint32_t*)(0x3C200014)))
#define WHEELRX      (*((volatile uint32_t*)(0x3C200018)))
#define WHEELTX      (*((volatile uint32_t*)(0x3C20001C)))


/////AES/////
#define AESCONTROL   (*((volatile uint32_t*)(0x39800000)))
#define AESGO        (*((volatile uint32_t*)(0x39800004)))
#define AESUNKREG0   (*((volatile uint32_t*)(0x39800008)))
#define AESSTATUS    (*((volatile uint32_t*)(0x3980000C)))
#define AESUNKREG1   (*((volatile uint32_t*)(0x39800010)))
#define AESKEYLEN    (*((volatile uint32_t*)(0x39800014)))
#define AESOUTSIZE   (*((volatile uint32_t*)(0x39800018)))
#define AESOUTADDR   (*((volatile uint32_t*)(0x39800020)))
#define AESINSIZE    (*((volatile uint32_t*)(0x39800024)))
#define AESINADDR    (*((volatile uint32_t*)(0x39800028)))
#define AESAUXSIZE   (*((volatile uint32_t*)(0x3980002C)))
#define AESAUXADDR   (*((volatile uint32_t*)(0x39800030)))
#define AESSIZE3     (*((volatile uint32_t*)(0x39800034)))
#define AESKEY         ((volatile uint32_t*)(0x3980004C))
#define AESTYPE      (*((volatile uint32_t*)(0x3980006C)))
#define AESIV          ((volatile uint32_t*)(0x39800074))
#define AESTYPE2     (*((volatile uint32_t*)(0x39800088)))
#define AESUNKREG2   (*((volatile uint32_t*)(0x3980008C)))

/////HASH/////
#define HASHCTRL     (*((volatile uint32_t*)(0x3C600000)))
#define HASHRESULT     ((volatile uint32_t*)(0x3C600020))
#define HASHDATAIN     ((volatile uint32_t*)(0x3C600040))


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
uint64_t read_42nsec_timer();
#define FOURTYTWO_NSEC_TIMER (read_42nsec_timer()) /* 64bit 24MHz timer */


/////USB/////
#define OTGBASE 0x38800000
#define PHYBASE 0x3C400000


/////I2C/////
#define IICCON       (*((volatile uint32_t*)(0x3C900000)))
#define IICSTAT      (*((volatile uint32_t*)(0x3C900004)))
#define IICADD       (*((volatile uint32_t*)(0x3C900008)))
#define IICDS        (*((volatile uint32_t*)(0x3C90000C)))


/////INTERRUPTS/////
#define VICIRQSTATUS(v)       (*((volatile uint32_t*)(0x38E00000 + 0x1000 * v)))
#define VICFIQSTATUS(v)       (*((volatile uint32_t*)(0x38E00004 + 0x1000 * v)))
#define VICRAWINTR(v)         (*((volatile uint32_t*)(0x38E00008 + 0x1000 * v)))
#define VICINTSELECT(v)       (*((volatile uint32_t*)(0x38E0000C + 0x1000 * v)))
#define VICINTENABLE(v)       (*((volatile uint32_t*)(0x38E00010 + 0x1000 * v)))
#define VICINTENCLEAR(v)      (*((volatile uint32_t*)(0x38E00014 + 0x1000 * v)))
#define VICSOFTINT(v)         (*((volatile uint32_t*)(0x38E00018 + 0x1000 * v)))
#define VICSOFTINTCLEAR(v)    (*((volatile uint32_t*)(0x38E0001C + 0x1000 * v)))
#define VICPROTECTION(v)      (*((volatile uint32_t*)(0x38E00020 + 0x1000 * v)))
#define VICSWPRIORITYMASK(v)  (*((volatile uint32_t*)(0x38E00024 + 0x1000 * v)))
#define VICPRIORITYDAISY(v)   (*((volatile uint32_t*)(0x38E00028 + 0x1000 * v)))
#define VICVECTADDR(v, i)     (*((volatile uint32_t*)(0x38E00100 + 0x1000 * v + 4 * i)))
#define VICVECTPRIORITY(v, i) (*((volatile uint32_t*)(0x38E00200 + 0x1000 * v + 4 * i)))
#define VICADDRESS(v)         (*((volatile uint32_t*)(0x38E00F00 + 0x1000 * v)))
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
#define VIC0VECTADDR(i)       (*((volatile uint32_t*)(0x38E00100 + 4 * i)))
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
#define VIC0VECTPRIORITY(i)   (*((volatile uint32_t*)(0x38E00200 + 4 * i)))
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
#define VIC1VECTADDR(i)       (*((volatile uint32_t*)(0x38E01100 + 4 * i)))
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
#define VIC1VECTPRIORITY(i)   (*((volatile uint32_t*)(0x38E01200 + 4 * i)))
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
#define INTMSK_TIMER   (1<<8)
#define INTMSK_USB_OTG (1<<23)


#endif
