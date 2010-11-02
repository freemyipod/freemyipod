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


#include "global.h"
#include "panic.h"
#include "interrupt.h"
#include "s5l8701.h"


#define default_interrupt(name) extern __attribute__((weak,alias("unhandled_irq"))) void name(void)

default_interrupt(EXT0);
default_interrupt(EXT1);
default_interrupt(EXT2);
default_interrupt(EINT_VBUS);
default_interrupt(EINTG);
default_interrupt(INT_WDT);
default_interrupt(INT_TIMERA);
default_interrupt(INT_TIMERB);
default_interrupt(INT_TIMERC);
default_interrupt(INT_TIMERD);
default_interrupt(INT_DMA0);
default_interrupt(INT_DMA1);
default_interrupt(INT_DMA2);
default_interrupt(INT_DMA3);
default_interrupt(INT_DMA4);
default_interrupt(INT_DMA5);
default_interrupt(INT_DMA6);
default_interrupt(INT_DMA7);
default_interrupt(INT_DMA8);
default_interrupt(INT_ALARM_RTC);
default_interrupt(INT_PRI_RTC);
default_interrupt(RESERVED1);
default_interrupt(INT_UART);
default_interrupt(INT_USB_HOST);
default_interrupt(INT_USB_FUNC);
default_interrupt(INT_LCDC_0);
default_interrupt(INT_LCDC_1);
default_interrupt(INT_ECC);
default_interrupt(INT_CALM);
default_interrupt(INT_ATA);
default_interrupt(INT_UART0);
default_interrupt(INT_SPDIF_OUT);
default_interrupt(INT_SDCI);
default_interrupt(INT_LCD);
default_interrupt(INT_WHEEL);
default_interrupt(INT_IIC);
default_interrupt(RESERVED2);
default_interrupt(INT_MSTICK);
default_interrupt(INT_ADC_WAKEUP);
default_interrupt(INT_ADC);
default_interrupt(INT_UNK1);
default_interrupt(INT_UNK2);
default_interrupt(INT_UNK3);


void unhandled_irq(void)
{
    panicf(PANIC_FATAL, "Unhandled IRQ %d!", INTOFFSET);
}

static void (* timervector[])(void) IDATA_ATTR =
{
    INT_TIMERA,INT_TIMERB,INT_TIMERC,INT_TIMERD
};

void INT_TIMER(void) ICODE_ATTR;
void INT_TIMER()
{
    if (TACON & 0x00038000) timervector[0]();
    if (TBCON & 0x00038000) timervector[1]();
    if (TCCON & 0x00038000) timervector[2]();
    if (TDCON & 0x00038000) timervector[3]();
}

static void (* const dmavector[])(void) IDATA_ATTR =
{
    INT_DMA0,INT_DMA1,INT_DMA2,INT_DMA3,INT_DMA4,INT_DMA5,INT_DMA6,INT_DMA7,INT_DMA8
};

void INT_DMA(void) ICODE_ATTR;
void INT_DMA()
{
    uint32_t dmaallst = DMAALLST;
    uint32_t dmaallst2 = DMAALLST2;
    if (dmaallst & (DMACON0 >> 16) & 3) dmavector[0]();
    if (dmaallst & (DMACON1 >> 12) & 0x30) dmavector[1]();
    if (dmaallst & (DMACON2 >> 8) & 0x300) dmavector[2]();
    if (dmaallst & (DMACON3 >> 4) & 0x3000) dmavector[3]();
    if (dmaallst2 & (DMACON4 >> 16) & 3) dmavector[4]();
    if (dmaallst2 & (DMACON5 >> 12) & 0x30) dmavector[5]();
    if (dmaallst2 & (DMACON6 >> 8) & 0x300) dmavector[6]();
    if (dmaallst2 & (DMACON7 >> 4) & 0x3000) dmavector[7]();
    if (dmaallst2 & DMACON8 & 0x30000) dmavector[8]();
}

static void (* irqvector[])(void) IDATA_ATTR =
{
    EXT0,EXT1,EXT2,EINT_VBUS,EINTG,INT_TIMER,INT_WDT,INT_UNK1,
    INT_UNK2,INT_UNK3,INT_DMA,INT_ALARM_RTC,INT_PRI_RTC,RESERVED1,INT_UART,INT_USB_HOST,
    INT_USB_FUNC,INT_LCDC_0,INT_LCDC_1,INT_CALM,INT_ATA,INT_UART0,INT_SPDIF_OUT,INT_ECC,
    INT_SDCI,INT_LCD,INT_WHEEL,INT_IIC,RESERVED2,INT_MSTICK,INT_ADC_WAKEUP,INT_ADC
};

void irqhandler(void)
{
    int irq_no = INTOFFSET;
    irqvector[irq_no]();
    SRCPND = (1 << irq_no);
    INTPND = INTPND;
}

void interrupt_enable(int irq, bool state)
{
	if (state) INTMSK |= 1 << irq;
	else INTMSK &= ~(1 << irq);
}

void interrupt_set_handler(int irq, void* handler)
{
	if (handler) irqvector[irq] = handler;
	else irqvector[irq] = unhandled_irq;
}

void int_timer_set_handler(int timer, void* handler)
{
	if (handler) timervector[timer] = handler;
	else timervector[timer] = unhandled_irq;
}

void interrupt_init(void)
{
    INTMSK = (1 << IRQ_TIMER) | (1 << IRQ_DMA);
}

void interrupt_shutdown(void)
{
    INTMSK = 0;
}
