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
#include "s5l8702.h"


#define default_interrupt(name) extern __attribute__((weak,alias("unhandled_irq"))) void name(void)

default_interrupt(INT_IRQ0);
default_interrupt(INT_IRQ1);
default_interrupt(INT_IRQ2);
default_interrupt(INT_IRQ3);
default_interrupt(INT_IRQ4);
default_interrupt(INT_IRQ5);
default_interrupt(INT_IRQ6);
default_interrupt(INT_IRQ7);
default_interrupt(INT_TIMERA);
default_interrupt(INT_TIMERB);
default_interrupt(INT_TIMERC);
default_interrupt(INT_TIMERD);
default_interrupt(INT_TIMERE);
default_interrupt(INT_TIMERF);
default_interrupt(INT_TIMERG);
default_interrupt(INT_TIMERH);
default_interrupt(INT_IRQ9);
default_interrupt(INT_IRQ10);
default_interrupt(INT_IRQ11);
default_interrupt(INT_IRQ12);
default_interrupt(INT_IRQ13);
default_interrupt(INT_IRQ14);
default_interrupt(INT_IRQ15);
default_interrupt(INT_DMAC0C0);
default_interrupt(INT_DMAC0C1);
default_interrupt(INT_DMAC0C2);
default_interrupt(INT_DMAC0C3);
default_interrupt(INT_DMAC0C4);
default_interrupt(INT_DMAC0C5);
default_interrupt(INT_DMAC0C6);
default_interrupt(INT_DMAC0C7);
default_interrupt(INT_DMAC1C0);
default_interrupt(INT_DMAC1C1);
default_interrupt(INT_DMAC1C2);
default_interrupt(INT_DMAC1C3);
default_interrupt(INT_DMAC1C4);
default_interrupt(INT_DMAC1C5);
default_interrupt(INT_DMAC1C6);
default_interrupt(INT_DMAC1C7);
default_interrupt(INT_IRQ18);
default_interrupt(INT_USB_FUNC);
default_interrupt(INT_IRQ20);
default_interrupt(INT_IRQ21);
default_interrupt(INT_IRQ22);
default_interrupt(INT_IRQ23);
default_interrupt(INT_IRQ24);
default_interrupt(INT_IRQ25);
default_interrupt(INT_IRQ26);
default_interrupt(INT_IRQ27);
default_interrupt(INT_IRQ28);
default_interrupt(INT_IRQ29);
default_interrupt(INT_IRQ30);
default_interrupt(INT_IRQ31);
default_interrupt(INT_IRQ32);
default_interrupt(INT_IRQ33);
default_interrupt(INT_IRQ34);
default_interrupt(INT_IRQ35);
default_interrupt(INT_IRQ36);
default_interrupt(INT_IRQ37);
default_interrupt(INT_IRQ38);
default_interrupt(INT_IRQ39);
default_interrupt(INT_IRQ40);
default_interrupt(INT_IRQ41);
default_interrupt(INT_IRQ42);
default_interrupt(INT_IRQ43);
default_interrupt(INT_IRQ44);
default_interrupt(INT_IRQ45);
default_interrupt(INT_IRQ46);
default_interrupt(INT_IRQ47);
default_interrupt(INT_IRQ48);
default_interrupt(INT_IRQ49);
default_interrupt(INT_IRQ50);
default_interrupt(INT_IRQ51);
default_interrupt(INT_IRQ52);
default_interrupt(INT_IRQ53);
default_interrupt(INT_IRQ54);
default_interrupt(INT_IRQ55);
default_interrupt(INT_IRQ56);
default_interrupt(INT_IRQ57);
default_interrupt(INT_IRQ58);
default_interrupt(INT_IRQ59);
default_interrupt(INT_IRQ60);
default_interrupt(INT_IRQ61);
default_interrupt(INT_IRQ62);
default_interrupt(INT_IRQ63);


static int current_irq;


void unhandled_irq(void)
{
    panicf(PANIC_FATAL, "Unhandled IRQ %d!", current_irq);
}

static void (* timervector[])(void) IDATA_ATTR =
{
    INT_TIMERA,INT_TIMERB,INT_TIMERC,INT_TIMERD,INT_TIMERE,INT_TIMERF,INT_TIMERG,INT_TIMERH
};

void INT_TIMER(void) ICODE_ATTR;
void INT_TIMER()
{
    if (TACON & 0x00038000) timervector[0]();
    if (TBCON & 0x00038000) timervector[1]();
    if (TCCON & 0x00038000) timervector[2]();
    if (TDCON & 0x00038000) timervector[3]();
    if (TECON & 0x00038000) timervector[4]();
    if (TFCON & 0x00038000) timervector[5]();
    if (TGCON & 0x00038000) timervector[6]();
    if (THCON & 0x00038000) timervector[7]();
}

static void (* dmavector[])(void) IDATA_ATTR =
{
    INT_DMAC0C0,INT_DMAC0C1,INT_DMAC0C2,INT_DMAC0C3,INT_DMAC0C4,INT_DMAC0C5,INT_DMAC0C6,INT_DMAC0C7,
    INT_DMAC1C0,INT_DMAC1C1,INT_DMAC1C2,INT_DMAC1C3,INT_DMAC1C4,INT_DMAC1C5,INT_DMAC1C6,INT_DMAC1C7
};

void INT_DMAC0(void) ICODE_ATTR;
void INT_DMAC0()
{
    uint32_t intsts = DMAC0INTSTS;
    if (intsts & 1) dmavector[0]();
    if (intsts & 2) dmavector[1]();
    if (intsts & 4) dmavector[2]();
    if (intsts & 8) dmavector[3]();
    if (intsts & 0x10) dmavector[4]();
    if (intsts & 0x20) dmavector[5]();
    if (intsts & 0x40) dmavector[6]();
    if (intsts & 0x80) dmavector[7]();
}

void INT_DMAC1(void) ICODE_ATTR;
void INT_DMAC1()
{
    uint32_t intsts = DMAC1INTSTS;
    if (intsts & 1) dmavector[8]();
    if (intsts & 2) dmavector[9]();
    if (intsts & 4) dmavector[10]();
    if (intsts & 8) dmavector[11]();
    if (intsts & 0x10) dmavector[12]();
    if (intsts & 0x20) dmavector[13]();
    if (intsts & 0x40) dmavector[14]();
    if (intsts & 0x80) dmavector[15]();
}

static void (* irqvector[])(void) IDATA_ATTR =
{
    INT_IRQ0,INT_IRQ1,INT_IRQ2,INT_IRQ3,INT_IRQ4,INT_IRQ5,INT_IRQ6,INT_IRQ7,
    INT_TIMER,INT_IRQ9,INT_IRQ10,INT_IRQ11,INT_IRQ12,INT_IRQ13,INT_IRQ14,INT_IRQ15,
    INT_DMAC0,INT_DMAC1,INT_IRQ18,INT_USB_FUNC,INT_IRQ20,INT_IRQ21,INT_IRQ22,INT_IRQ23,
    INT_IRQ24,INT_IRQ25,INT_IRQ26,INT_IRQ27,INT_IRQ28,INT_IRQ29,INT_IRQ30,INT_IRQ31,
    INT_IRQ32,INT_IRQ33,INT_IRQ34,INT_IRQ35,INT_IRQ36,INT_IRQ37,INT_IRQ38,INT_IRQ39,
    INT_IRQ40,INT_IRQ41,INT_IRQ42,INT_IRQ43,INT_IRQ55,INT_IRQ56,INT_IRQ57,INT_IRQ58,
    INT_IRQ48,INT_IRQ49,INT_IRQ50,INT_IRQ51,INT_IRQ52,INT_IRQ53,INT_IRQ54,INT_IRQ55,
    INT_IRQ56,INT_IRQ57,INT_IRQ58,INT_IRQ59,INT_IRQ60,INT_IRQ61,INT_IRQ62,INT_IRQ63
};

void irqhandler(void)
{
    void* dummy = VIC0ADDRESS;
    dummy = VIC1ADDRESS;
    uint32_t irqs0 = VIC0IRQSTATUS;
    uint32_t irqs1 = VIC1IRQSTATUS;
    for (current_irq = 0; irqs0; current_irq++, irqs0 >>= 1)
        if (irqs0 & 1)
            irqvector[current_irq]();
    for (current_irq = 32; irqs1; current_irq++, irqs1 >>= 1)
        if (irqs1 & 1)
            irqvector[current_irq]();
    VIC0ADDRESS = NULL;
    VIC1ADDRESS = NULL;
}

void interrupt_enable(int irq, bool state)
{
    if (state) VICINTENABLE(irq >> 5) = 1 << (irq & 0x1f);
    else VICINTENCLEAR(irq >> 5) = 1 << (irq & 0x1f);
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

void int_dma_set_handler(int channel, void* handler)
{
    if (handler) dmavector[channel] = handler;
    else dmavector[channel] = unhandled_irq;
}

void interrupt_init(void)
{
    VIC0INTENABLE = 1 << IRQ_DMAC0;
    VIC0INTENABLE = 1 << IRQ_DMAC1;
}

void interrupt_shutdown(void)
{
    VIC0INTENCLEAR = 0xffffffff;
    VIC1INTENCLEAR = 0xffffffff;
}
