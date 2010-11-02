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
#include "mmu.h"
#include "panic.h"
#include "usbdrv.h"
#include "thread.h"
#include "timer.h"
#include "usb.h"
#include "usb_ch9.h"
#include "synopsysotg.h"
#include "util.h"
#include "interrupt.h"
#include "clockgates.h"
#include "power.h"


struct ep_type
{
    bool active;
    bool busy;
    bool done;
    int rc;
    int size;
    struct wakeup complete;
} ;

static struct ep_type endpoints[5];
static struct usb_ctrlrequest ctrlreq CACHEALIGN_ATTR;
static uint32_t synopsysotg_stack[0x40] STACK_ATTR;

int usb_drv_port_speed(void)
{
    return (DSTS & 2) == 0 ? 1 : 0;
}

static void reset_endpoints(int reinit)
{
    unsigned int i;
    for (i = 0; i < sizeof(endpoints)/sizeof(struct ep_type); i++)
    {
        if (reinit) endpoints[i].active = false;
        endpoints[i].busy = false;
        endpoints[i].rc = -1;
        endpoints[i].done = true;
        wakeup_signal(&endpoints[i].complete);
    }
    DIEPCTL0 = 0x8800;  /* EP0 IN ACTIVE NEXT=1 */
    DOEPCTL0 = 0x8000;  /* EP0 OUT ACTIVE */
    DOEPTSIZ0 = 0x20080040;  /* EP0 OUT Transfer Size:
                                64 Bytes, 1 Packet, 1 Setup Packet */
    DOEPDMA0 = (uint32_t)&ctrlreq;
    DOEPCTL0 |= 0x84000000;  /* EP0 OUT ENABLE CLEARNAK */
    if (reinit)
    {
        /* The size is getting set to zero, because we don't know
           whether we are Full Speed or High Speed at this stage */
        /* EP1 IN INACTIVE DATA0 SIZE=0 NEXT=3 */
        DIEPCTL1 = 0x10001800;
        /* EP2 OUT INACTIVE DATA0 SIZE=0 */
        DOEPCTL2 = 0x10000000;
        /* EP3 IN INACTIVE DATA0 SIZE=0 NEXT=0 */
        DIEPCTL3 = 0x10000000;
        /* EP4 OUT INACTIVE DATA0 SIZE=0 */
        DOEPCTL4 = 0x10000000;
    }
    else
    {
        /* INACTIVE DATA0 */
        DIEPCTL1 = (DIEPCTL1 & ~0x00008000) | 0x10000000;
        DOEPCTL2 = (DOEPCTL2 & ~0x00008000) | 0x10000000;
        DIEPCTL3 = (DIEPCTL3 & ~0x00008000) | 0x10000000;
        DOEPCTL4 = (DOEPCTL4 & ~0x00008000) | 0x10000000;
    }
    DAINTMSK = 0xFFFFFFFF;  /* Enable interrupts on all EPs */
}

int usb_drv_request_endpoint(int type, int dir)
{
    size_t ep;
    int ret = -1;

    if (dir == USB_DIR_IN) ep = 1;
    else ep = 2;

    while (ep < 5)
    {
        if (!endpoints[ep].active)
        {
            endpoints[ep].active = true;
            ret = ep | dir;
            uint32_t newbits = (type << 18) | 0x10000000;
            if (dir) DIEPCTL(ep) = (DIEPCTL(ep) & ~0x000C0000) | newbits;
            else DOEPCTL(ep) = (DOEPCTL(ep) & ~0x000C0000) | newbits;
            break;
        }
        ep += 2;
    }

    return ret;
}

void usb_drv_release_endpoint(int ep)
{
    ep = ep & 0x7f;

    if (ep < 1 || ep > USB_NUM_ENDPOINTS) return;

    endpoints[ep].active = false;
}

static void usb_reset(void)
{
    volatile int i;

    DCTL = 0x802;  /* Soft Disconnect */

    OPHYPWR = 0;  /* PHY: Power up */
    OPHYUNK1 = 1;
    OPHYUNK2 = 0xE3F;
    OPHYCLK = SYNOPSYSOTG_CLOCK;
    ORSTCON = 1;  /* PHY: Assert Software Reset */
    udelay(10);
    ORSTCON = 0;  /* PHY: Deassert Software Reset */

    GRSTCTL = 1;  /* OTG: Assert Software Reset */
    while (GRSTCTL & 1);  /* Wait for OTG to ack reset */
    while (!(GRSTCTL & 0x80000000));  /* Wait for OTG AHB master idle */

    GRXFSIZ = 0x00000200;  /* RX FIFO: 512 bytes */
    GNPTXFSIZ = 0x02000200;  /* Non-periodic TX FIFO: 512 bytes */
    GAHBCFG = SYNOPSYSOTG_AHBCFG;
    GUSBCFG = 0x1408;  /* OTG: 16bit PHY and some reserved bits */

    DCFG = 4;  /* Address 0 */
    DCTL = 0x800;  /* Soft Reconnect */
    DIEPMSK = 0x0D;  /* IN EP interrupt mask */
    DOEPMSK = 0x0D;  /* IN EP interrupt mask */
    DAINTMSK = 0xFFFFFFFF;  /* Enable interrupts on all endpoints */
    GINTMSK = 0xC3000;  /* Interrupt mask: IN event, OUT event, bus reset */

    reset_endpoints(1);
}

/* IRQ handler */
void INT_USB_FUNC(void)
{
    int i;
    uint32_t ints = GINTSTS;
    uint32_t epints;
    if (ints & 0x1000)  /* bus reset */
    {
        DCFG = 4;  /* Address 0 */
        reset_endpoints(1);
        usb_handle_bus_reset();
    }

    if (ints & 0x2000)  /* enumeration done, we now know the speed */
    {
        /* Set up the maximum packet sizes accordingly */
        uint32_t maxpacket = usb_drv_port_speed() ? 512 : 64;
        DIEPCTL1 = (DIEPCTL1 & ~0x000003FF) | maxpacket;
        DOEPCTL2 = (DOEPCTL2 & ~0x000003FF) | maxpacket;
        DIEPCTL3 = (DIEPCTL3 & ~0x000003FF) | maxpacket;
        DOEPCTL4 = (DOEPCTL4 & ~0x000003FF) | maxpacket;
    }

    if (ints & 0x40000)  /* IN EP event */
        for (i = 0; i < 4; i += i + 1)  // 0, 1, 3
            if (epints = DIEPINT(i))
            {
                if (epints & 1)  /* Transfer completed */
                {
                    invalidate_dcache();
                    int bytes = endpoints[i].size - (DIEPTSIZ(i) & 0x3FFFF);
                    if (endpoints[i].busy)
                    {
                        endpoints[i].busy = false;
                        endpoints[i].rc = 0;
                        endpoints[i].done = true;
                        usb_handle_transfer_complete(i, USB_DIR_IN, 0, bytes);
                        wakeup_signal(&endpoints[i].complete);
                    }
                }
                if (epints & 4)  /* AHB error */
                    panicf(PANIC_FATAL, "USB: AHB error on IN EP%d", i);
                if (epints & 8)  /* Timeout */
                {
                    if (endpoints[i].busy)
                    {
                        endpoints[i].busy = false;
                        endpoints[i].rc = 1;
                        endpoints[i].done = true;
                        wakeup_signal(&endpoints[i].complete);
                    }
                }
                DIEPINT(i) = epints;
            }

    if (ints & 0x80000)  /* OUT EP event */
        for (i = 0; i < 5; i += 2)  // 0, 2, 4
            if (epints = DOEPINT(i))
            {
                if (epints & 1)  /* Transfer completed */
                {
                    invalidate_dcache();
                    int bytes = endpoints[i].size - (DOEPTSIZ(i) & 0x3FFFF);
                    if (endpoints[i].busy)
                    {
                        endpoints[i].busy = false;
                        endpoints[i].rc = 0;
                        endpoints[i].done = true;
                        usb_handle_transfer_complete(i, USB_DIR_OUT, 0, bytes);
                        wakeup_signal(&endpoints[i].complete);
                    }
                }
                if (epints & 4)  /* AHB error */
                    panicf(PANIC_FATAL, "USB: AHB error on OUT EP%d", i);
                if (epints & 8)  /* SETUP phase done */
                {
                    invalidate_dcache();
                    if (i == 0) usb_handle_control_request(&ctrlreq);
                    else panicf(PANIC_FATAL, "USB: SETUP done on OUT EP%d!?", i);
                }
                /* Make sure EP0 OUT is set up to accept the next request */
                if (!i)
                {
                    DOEPTSIZ0 = 0x20080040;
                    DOEPDMA0 = (uint32_t)&ctrlreq;
                    DOEPCTL0 |= 0x84000000;
                }
                DOEPINT(i) = epints;
            }

    GINTSTS = ints;
}

void usb_drv_set_address(int address)
{
    DCFG = (DCFG & ~0x7F0) | (address << 4);
}

static void ep_send(int ep, const void *ptr, int length)
{
    endpoints[ep].busy = true;
    endpoints[ep].size = length;
    DIEPCTL(ep) |= 0x8000;  /* EPx OUT ACTIVE */
    int blocksize = usb_drv_port_speed() ? 512 : 64;
    int packets = (length + blocksize - 1) / blocksize;
    if (!length)
    {
        DIEPTSIZ(ep) = 1 << 19;  /* one empty packet */
        DIEPDMA(ep) = 0x10000000;  /* dummy address */
    }
    else
    {
        DIEPTSIZ(ep) = length | (packets << 19);
        DIEPDMA(ep) = (uint32_t)ptr;
    }
    clean_dcache();
    DIEPCTL(ep) |= 0x84000000;  /* EPx OUT ENABLE CLEARNAK */
}

static void ep_recv(int ep, void *ptr, int length)
{
    endpoints[ep].busy = true;
    endpoints[ep].size = length;
    DOEPCTL(ep) &= ~0x20000;  /* EPx UNSTALL */
    DOEPCTL(ep) |= 0x8000;  /* EPx OUT ACTIVE */
    int blocksize = usb_drv_port_speed() ? 512 : 64;
    int packets = (length + blocksize - 1) / blocksize;
    if (!length)
    {
        DOEPTSIZ(ep) = 1 << 19;  /* one empty packet */
        DOEPDMA(ep) = 0x10000000;  /* dummy address */
    }
    else
    {
        DOEPTSIZ(ep) = length | (packets << 19);
        DOEPDMA(ep) = (uint32_t)ptr;
    }
    clean_dcache();
    DOEPCTL(ep) |= 0x84000000;  /* EPx OUT ENABLE CLEARNAK */
}

int usb_drv_send(int endpoint, const void *ptr, int length)
{
    endpoint &= 0x7f;
    endpoints[endpoint].done = false;
    ep_send(endpoint, ptr, length);
    while (!endpoints[endpoint].done && endpoints[endpoint].busy)
        wakeup_wait(&endpoints[endpoint].complete, TIMEOUT_BLOCK);
    return endpoints[endpoint].rc;
}

int usb_drv_send_nonblocking(int endpoint, const void *ptr, int length)
{
    ep_send(endpoint & 0x7f, ptr, length);
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    ep_recv(endpoint & 0x7f, ptr, length);
    return 0;
}

void usb_drv_cancel_all_transfers(void)
{
    uint32_t mode = enter_critical_section();
    reset_endpoints(0);
    leave_critical_section(mode);
}

bool usb_drv_stalled(int endpoint, bool in)
{
    if (in) return DIEPCTL(endpoint) & 0x00200000 ? true : false;
    else return DOEPCTL(endpoint) & 0x00200000 ? true : false;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    if (in)
    {
        if (stall) DIEPCTL(endpoint) |= 0x00200000;
        else DIEPCTL(endpoint) &= ~0x00200000;
    }
    else
    {
        if (stall) DOEPCTL(endpoint) |= 0x00200000;
        else DOEPCTL(endpoint) &= ~0x00200000;
    }
}

void usb_drv_power_up(void)
{
    /* Enable USB clock */
    clockgate_enable(CLOCKGATE_USB_1, true);
    clockgate_enable(CLOCKGATE_USB_2, true);
    PCGCCTL = 0;

    /* reset the beast */
    usb_reset();
}

void usb_drv_power_down(void)
{
    DCTL = 0x802;  /* Soft Disconnect */

    ORSTCON = 1;  /* Put the PHY into reset (needed to get current down) */
    PCGCCTL = 1;  /* Shut down PHY clock */
    OPHYPWR = 0xF;  /* PHY: Power down */
    
    clockgate_enable(CLOCKGATE_USB_1, false);
    clockgate_enable(CLOCKGATE_USB_2, false);
}

void usb_check_vbus()
{
    bool oldstate = false;
    while (true)
    {
        sleep(200000);
        bool newstate = vbus_state();
        if (oldstate != newstate)
        {
            if (newstate) usb_drv_power_up();
            else usb_drv_power_down();
            oldstate = newstate;
        }
    }
}

void usb_drv_init(void)
{
    unsigned int i;
    for (i = 0; i < sizeof(endpoints)/sizeof(struct ep_type); i++)
        wakeup_init(&endpoints[i].complete);

    /* Enable USB clock */
    clockgate_enable(CLOCKGATE_USB_1, true);
    clockgate_enable(CLOCKGATE_USB_2, true);
    PCGCCTL = 0;

    /* unmask irq */
    interrupt_enable(IRQ_USB_FUNC, true);

    thread_create("synopsysotg", usb_check_vbus, synopsysotg_stack,
                  sizeof(synopsysotg_stack), OS_THREAD, 63, true);

    usb_drv_power_down();
}

void usb_drv_exit(void)
{
    usb_drv_power_down();
}

int usb_drv_get_max_out_size()
{
    return usb_drv_port_speed() ? 262144 : 32768;
}

int usb_drv_get_max_in_size()
{
    return usb_drv_port_speed() ? 262144 : 32768;
}
