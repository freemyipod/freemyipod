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
#define PWRMODE      (*((volatile uint32_t*)(0x3C50002C)))
#define SWRCON       (*((volatile uint32_t*)(0x3C500030)))
#define RSTSR        (*((volatile uint32_t*)(0x3C500034)))
#define DSPCLKMD     (*((volatile uint32_t*)(0x3C500038)))
#define CLKCON2      (*((volatile uint32_t*)(0x3C50003C)))
#define PWRCON(i)    (*((volatile uint32_t*)(0x3C500000 + ((i) == 1 ? 0x40 : 0x28))))


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
#define PCON14       (*((volatile uint32_t*)(0x3CF000E0)))
#define PDAT14       (*((volatile uint32_t*)(0x3CF000E4)))
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


/////USB/////
#define OTGBASE 0x38800000
#define PHYBASE 0x3C400000
#define SYNOPSYSOTG_CLOCK 0
#define SYNOPSYSOTG_AHBCFG 0x27


/////I2C/////
#define IICCON       (*((volatile uint32_t*)(0x3C900000)))
#define IICSTAT      (*((volatile uint32_t*)(0x3C900004)))
#define IICADD       (*((volatile uint32_t*)(0x3C900008)))
#define IICDS        (*((volatile uint32_t*)(0x3C90000C)))


/////CLOCK GATES/////
#define CLOCKGATE_USB_1 14
#define CLOCKGATE_USB_2 43


/////INTERRUPTS/////
#define IRQ_TIMER    5
#define IRQ_DMA      10
#define IRQ_USB_FUNC 16
#define IRQ_ECC      19
#define IRQ_WHEEL    26
#define IRQ_IIC      27


#endif
