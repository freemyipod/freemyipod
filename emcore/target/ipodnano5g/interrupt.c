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

//    iPod nano 5G interrupt dispatch, adapted from the nano 4G driver:
//     - timers 4..7 report through TSTAT47 0x3c700118 (bits (0x38 - 8*i) + {0,1,2}), timers 0..3 through TCON bits 16..18;
//     - after each handler the stock code writes 1 << n to 0x38e02008 (VIC0) / 0x38e0200c (VIC1) (fw_disk:0x08044054);
//     - DMAC1 is at 0x38700000 (s5l8730.h); IRQ lines: timers 8, DMAC0/1 16/17, USB 19 (names in s5l8730.h).

#include "global.h"
#include "panic.h"
#include "interrupt.h"
#include "s5l8730.h"


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

/* Unhandled IRQ handling: an IRQ nothing services is recorded and MASKED instead of panicking,
   so the USB monitor stays alive. Hosts read dbg_unhandled_irq_last / dbg_unhandled_irq_count via memory-read. */
volatile uint32_t dbg_unhandled_irq_last = 0xffffffffu;
volatile uint32_t dbg_unhandled_irq_count = 0;

void unhandled_irq(void)
{
    dbg_unhandled_irq_last = (uint32_t)current_irq;
    dbg_unhandled_irq_count++;
    interrupt_enable(current_irq, false);   /* mask: never let it fire again */
}

static void (* timervector[])(void) IDATA_ATTR =
{
    INT_TIMERA,INT_TIMERB,INT_TIMERC,INT_TIMERD,INT_TIMERE,INT_TIMERF,INT_TIMERG,INT_TIMERH
};

/* timers 0..3 status = TCON bits 16..18 (enables 12..14); timers 4..7 status = TSTAT47 bits
   (0x38 - 8*i) + {0,1,2} (w1c), enables in their own TCON bits 12..14 (fw_disk ISR 0x0802f51c). */
void INT_TIMER(void) ICODE_ATTR;
void INT_TIMER()
{
    int i;
    for (i = 0; i < 4; i++)
    {
        int tcon = TCON(i);
        if (tcon & (tcon >> 4) & 0x7000) timervector[i]();
    }
    for (i = 4; i < 8; i++)
    {
        int shift = 0x38 - 8 * i;
        uint32_t sts = (TSTAT47 >> shift) & 7;
        uint32_t en = (*((uint32_t volatile*)(0x3C7000A0 + 0x20 * (i - 4))) >> 12) & 7;
        if (sts & en)
        {
            timervector[i]();
            TSTAT47 = sts << shift;
        }
    }
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
    INT_IRQ40,INT_IRQ41,INT_IRQ42,INT_IRQ43,INT_IRQ44,INT_IRQ45,INT_IRQ46,INT_IRQ47,
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
        {
            irqvector[current_irq]();
            VICEDGECLEAR(0) = 1u << current_irq;          /* fw_disk:0x08044054 / bl_InterruptController:0x394 */
        }
    for (current_irq = 32; irqs1; current_irq++, irqs1 >>= 1)
        if (irqs1 & 1)
        {
            irqvector[current_irq]();
            VICEDGECLEAR(1) = 1u << (current_irq - 32);
        }
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
    int i;
    /* A running LCD DMA channel must not have its CONTROL written (it freezes) nor its TC
       cleared (the completion IRQ is lost). Idle channels are cleared; DMAC1 (0x38700000) too. */
    for (i = 0; i < 8; i++) if (!(DMAC0CCONFIG(i) & 1)) DMAC0CCONTROL(i) = 0;
    for (i = 0; i < 8; i++) if (!(DMAC1CCONFIG(i) & 1)) DMAC1CCONTROL(i) = 0;
    DMAC0INTTCCLR = 0xff & ~0x10;
    DMAC0INTERRCLR = 0xff;
    DMAC1INTTCCLR = 0xff;
    DMAC1INTERRCLR = 0xff;
    VIC0INTSELECT = 0;          /* everything on IRQ, nothing on FIQ (bootrom:0x200013d8 does the same) */
    VIC1INTSELECT = 0;
    VIC0INTENABLE = 1 << IRQ_TIMER;
    VIC0INTENABLE = 1 << IRQ_DMAC0;
    VIC0INTENABLE = 1 << IRQ_DMAC1;
}

void interrupt_shutdown(void)
{
    VIC0INTENCLEAR = 0xffffffff;
    VIC1INTENCLEAR = 0xffffffff;
}
