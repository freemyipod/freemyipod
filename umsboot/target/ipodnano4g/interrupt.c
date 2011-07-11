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


#include "global.h"
#include "panic.h"
#include "interrupt.h"
#include "s5l8720.h"


#define default_interrupt(name) extern __attribute__((weak,alias("unhandled_irq"))) void name(void)

default_interrupt(INT_IRQ0);
default_interrupt(INT_IRQ1);
default_interrupt(INT_IRQ2);
default_interrupt(INT_IRQ3);
default_interrupt(INT_IRQ4);
default_interrupt(INT_IRQ5);
default_interrupt(INT_IRQ6);
default_interrupt(INT_IRQ7);
default_interrupt(INT_TIMER);
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
default_interrupt(INT_DMAC1);
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


static int current_irq;


void unhandled_irq(void)
{
    panicf(PANIC_FATAL, "Unhandled IRQ %d!", current_irq);
}

static void (* dmavector[])(void) IDATA_ATTR =
{
    INT_DMAC0C0,INT_DMAC0C1,INT_DMAC0C2,INT_DMAC0C3,INT_DMAC0C4,INT_DMAC0C5,INT_DMAC0C6,INT_DMAC0C7
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

static void (* irqvector[])(void) IDATA_ATTR =
{
    INT_IRQ0,INT_IRQ1,INT_IRQ2,INT_IRQ3,INT_IRQ4,INT_IRQ5,INT_IRQ6,INT_IRQ7,
    INT_TIMER,INT_IRQ9,INT_IRQ10,INT_IRQ11,INT_IRQ12,INT_IRQ13,INT_IRQ14,INT_IRQ15,
    INT_DMAC0,INT_DMAC1,INT_IRQ18,INT_USB_FUNC,INT_IRQ20,INT_IRQ21,INT_IRQ22,INT_IRQ23,
    INT_IRQ24,INT_IRQ25,INT_IRQ26,INT_IRQ27,INT_IRQ28,INT_IRQ29,INT_IRQ30,INT_IRQ31
};

void irqhandler(void)
{
    void* dummy = VIC0ADDRESS;
    uint32_t irqs0 = VIC0IRQSTATUS;
    for (current_irq = 0; irqs0; current_irq++, irqs0 >>= 1)
        if (irqs0 & 1)
            irqvector[current_irq]();
    VIC0ADDRESS = NULL;
}

void interrupt_enable(int irq, bool state)
{
    if (state) VICINTENABLE(0) = 1 << irq;
    else VICINTENCLEAR(0) = 1 << irq;
}

void interrupt_set_handler(int irq, void* handler)
{
    if (handler) irqvector[irq] = handler;
    else irqvector[irq] = unhandled_irq;
}

void int_dma_set_handler(int channel, void* handler)
{
    if (handler) dmavector[channel] = handler;
    else dmavector[channel] = unhandled_irq;
}

void interrupt_init(void)
{
    int i;
    for (i = 0; i < 8; i++) DMAC0CCONTROL(i) = 0;
    DMAC0INTTCCLR = 0xff;
    DMAC0INTERRCLR = 0xff;
    VIC0INTENABLE = 1 << IRQ_DMAC0;
}

void interrupt_shutdown(void)
{
    VIC0INTENCLEAR = 0xffffffff;
}
