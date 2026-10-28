#include "global.h"
#include "synopsysotg.h"
#include "usb.h"
#include "timer.h"
#include "util.h"
#include "mmu.h"
#ifdef TARGET_ipodnano5g
extern uint32_t wedge_fix_enable;
volatile uint32_t wedge_snap[14];
#define WEDGE_SNAP_IRQMAX   (wedge_snap[0])
#define WEDGE_SNAP_IRQCNT   (wedge_snap[1])
#define WEDGE_SNAP_IRQLAST  (wedge_snap[2])
#define WEDGE_SNAP_TXFREE   (wedge_snap[4])
#define WEDGE_SNAP_RXRES    (wedge_snap[5])
static inline void wedge_irq_hold_sample(uint32_t t0, uint32_t t1)
{
    uint32_t d = t1 - t0;
    if ((int32_t)d >= 0)
    {
        WEDGE_SNAP_IRQLAST = d;
        WEDGE_SNAP_IRQCNT++;
        if (d > WEDGE_SNAP_IRQMAX) WEDGE_SNAP_IRQMAX = d;
    }
}
static inline uint32_t wedge_txfifo_free(const struct synopsysotg_config* data, int ep)
{
    if (!ep) return data->core->gregs.hnptxsts.b.nptxfspcavail;
    return data->core->inep_regs[ep].dtxfsts.b.txfspcavail;
}
static inline uint32_t wedge_rx_resident(const struct synopsysotg_config* data)
{
    return (data->core->gregs.grxstsr.b.pktsts != 0u) ? 1u : 0u;
}
#endif
#ifndef SYNOPSYSOTG_AHB_BURST_LEN
#define SYNOPSYSOTG_AHB_BURST_LEN 5
#endif
#ifndef SYNOPSYSOTG_AHB_THRESHOLD
#define SYNOPSYSOTG_AHB_THRESHOLD 8
#endif
#ifndef SYNOPSYSOTG_TURNAROUND
#define SYNOPSYSOTG_TURNAROUND 3
#endif
static void synopsysotg_flush_out_endpoint(const struct usb_instance* instance, int ep)
{
    if (!ep) return;
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    if (data->core->outep_regs[ep].doepctl.b.epena)
    {
        synopsysotg_target_disable_irq(instance);
        data->core->dregs.dctl.b.sgoutnak = 1;
        while (!(data->core->gregs.gintsts.b.goutnakeff));
        data->core->outep_regs[ep].doepctl.b.snak = 1;
        data->core->outep_regs[ep].doepctl.b.epdis = 1;
        while (!(data->core->outep_regs[ep].doepint.b.epdisabled));
        data->core->dregs.dctl.b.cgoutnak = 1;
        synopsysotg_target_enable_irq(instance);
    }
    data->core->outep_regs[ep].doepctl.b.usbactep = 0;
    data->core->outep_regs[ep].doeptsiz.d32 = 0;
}
static void synopsysotg_flush_in_endpoint(const struct usb_instance* instance, int ep)
{
    if (!ep) return;
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    if (data->core->inep_regs[ep].diepctl.b.epena)
    {
        synopsysotg_target_disable_irq(instance);
        data->core->inep_regs[ep].diepctl.b.snak = 1;
        while (!(data->core->inep_regs[ep].diepint.b.inepnakeff));
        data->core->inep_regs[ep].diepctl.b.epdis = 1;
        while (!(data->core->inep_regs[ep].diepint.b.epdisabled));
        data->core->inep_regs[ep].diepctl.b.usbactep = 0;
        synopsysotg_target_enable_irq(instance);
        while (!data->core->gregs.grstctl.b.ahbidle);
        union synopsysotg_grstctl grstctl = { .b = { .txfnum = data->core->inep_regs[ep].diepctl.b.txfnum, .txfflsh = 1 } };
        data->core->gregs.grstctl = grstctl;
        while (data->core->gregs.grstctl.b.txfflsh);
    }
    else if (ep) data->core->inep_regs[ep].diepctl.b.usbactep = 0;
    data->core->inep_regs[ep].dieptsiz.d32 = 0;
}
#ifdef TARGET_ipodnano5g
static void synopsysotg_flush_txfifo(const struct usb_instance* instance, int ep)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    int i;
    for (i = 0; i < 1000 && !data->core->gregs.grstctl.b.ahbidle; i++) ;
    union synopsysotg_grstctl grstctl = { .b = { .txfnum = ep, .txfflsh = 1 } };
    data->core->gregs.grstctl = grstctl;
    for (i = 0; i < 1000 && data->core->gregs.grstctl.b.txfflsh; i++) ;
    for (i = 0; i < 1000 && !data->core->gregs.grstctl.b.ahbidle; i++) ;
}
static void synopsysotg_ep0_kill_stale_in(const struct usb_instance* instance)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    volatile struct synopsysotg_inepregs* regs = &data->core->inep_regs[0];
    int i;
    if (!regs->diepctl.b.epena) return;
    regs->diepctl.b.snak = 1;
    regs->diepctl.b.epdis = 1;
    for (i = 0; i < 1000 && !regs->diepint.b.epdisabled; i++) ;
    regs->diepint = regs->diepint;
    synopsysotg_flush_txfifo(instance, 0);
}
#endif
static void synopsysotg_flush_ints(const struct usb_instance* instance)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    int i;
    for (i = 0; i < 16; i++)
    {
        data->core->outep_regs[i].doepint = data->core->outep_regs[i].doepint;
        data->core->inep_regs[i].diepint = data->core->inep_regs[i].diepint;
    }
    data->core->gregs.gintsts = data->core->gregs.gintsts;
}
static void synopsysotg_try_push(const struct usb_instance* instance, int ep)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    struct synopsysotg_state* state = (struct synopsysotg_state*)instance->driver_state;
    union synopsysotg_depctl depctl = data->core->inep_regs[ep].diepctl;
    if (!depctl.b.epena || !depctl.b.usbactep || depctl.b.stall || depctl.b.naksts) return;
    int bytesleft = data->core->inep_regs[ep].dieptsiz.b.xfersize;
    if (!bytesleft) return;
    int maxpacket = ep ? data->core->inep_regs[ep].diepctl.b.mps : 64;
    union synopsysotg_hnptxsts fifospace = data->core->gregs.hnptxsts;
    int words = (MIN(maxpacket, bytesleft) + 3) >> 2;
    if (fifospace.b.nptxqspcavail && fifospace.b.nptxfspcavail << 2 >= words)
        while (words--) data->core->dfifo[ep][0] = *state->endpoints[ep].txaddr++;
    if (!words && bytesleft <= maxpacket) return;
    data->core->gregs.gintmsk.b.nptxfempty = true;
}
void synopsysotg_start_rx(const struct usb_instance* instance, union usb_endpoint_number ep, void* buf, int size)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    struct synopsysotg_state* state = (struct synopsysotg_state*)instance->driver_state;
    volatile struct synopsysotg_outepregs* regs = &data->core->outep_regs[ep.number];
    int maxpacket = regs->doepctl.b.mps;
    int packets = (size + maxpacket - 1) / maxpacket;
    if (!packets) packets = 1;
    if (data->use_dma) regs->doepdma = buf;
    else state->endpoints[ep.number].rxaddr = (uint32_t*)buf;
    union synopsysotg_depxfrsiz deptsiz = { .b = { .pktcnt = packets, .xfersize = size } };
    regs->doeptsiz = deptsiz;
#ifdef TARGET_ipodnano5g
    if (data->use_dma && (wedge_fix_enable & (1u << 2)) && wedge_rx_resident(data))
    {
        union synopsysotg_grstctl rxf = { .b = { .rxfflsh = 1 } };
        int i;
        data->core->gregs.grstctl = rxf;
        for (i = 0; i < 1000 && data->core->gregs.grstctl.b.rxfflsh; i++) ;
    }
    WEDGE_SNAP_RXRES = wedge_rx_resident(data);
    if (data->use_dma)
    {
        if (wedge_fix_enable & (1u << 0)) invalidate_dcache_range((uint32_t)buf, size);
        else invalidate_dcache(buf, size);
    }
#else
    if (data->use_dma) invalidate_dcache(buf, size);
#endif
    union synopsysotg_depctl depctl = regs->doepctl;
    depctl.b.epena = 1;
    depctl.b.cnak = 1;
    regs->doepctl = depctl;
}
void synopsysotg_start_tx(const struct usb_instance* instance, union usb_endpoint_number ep, const void* buf, int size)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    struct synopsysotg_state* state = (struct synopsysotg_state*)instance->driver_state;
    volatile struct synopsysotg_inepregs* regs = &data->core->inep_regs[ep.number];
#ifdef TARGET_ipodnano5g
    WEDGE_SNAP_TXFREE = wedge_txfifo_free(data, ep.number);
    if (data->use_dma && (wedge_fix_enable & (1u << 1)))
    {
        int depth = data->txfifosize[ep.number];
        uint32_t free = wedge_txfifo_free(data, ep.number);
        if (depth && free < (uint32_t)depth) synopsysotg_flush_txfifo(instance, ep.number);
    }
#endif
    int maxpacket = regs->diepctl.b.mps;
    int packets = (size + maxpacket - 1) / maxpacket;
    if (!packets) packets = 1;
    if (data->use_dma) regs->diepdma = buf;
    else state->endpoints[ep.number].txaddr = (uint32_t*)buf;
    union synopsysotg_depxfrsiz deptsiz = { .b = { .pktcnt = packets, .xfersize = size } };
    regs->dieptsiz = deptsiz;
#ifdef TARGET_ipodnano5g
    if (data->use_dma)
    {
        if (wedge_fix_enable & (1u << 0)) clean_dcache_range((uint32_t)buf, size);
        else clean_dcache(buf, size);
    }
#else
    if (data->use_dma) clean_dcache(buf, size);
#endif
    union synopsysotg_depctl depctl = regs->diepctl;
    depctl.b.epena = 1;
    depctl.b.cnak = 1;
    regs->diepctl = depctl;
    if (size && !data->use_dma)
    {
        if (data->shared_txfifo) synopsysotg_try_push(instance, ep.number);
        else data->core->dregs.diepempmsk.ep.in |= (1 << ep.number);
    }
}
int synopsysotg_get_stall(const struct usb_instance* instance, union usb_endpoint_number ep)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
        return !!data->core->inep_regs[ep.number].diepctl.b.stall;
    return !!data->core->outep_regs[ep.number].doepctl.b.stall;
}
void synopsysotg_set_stall(const struct usb_instance* instance, union usb_endpoint_number ep, bool stall)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        data->core->inep_regs[ep.number].diepctl.b.stall = !!stall;
        if (!stall) data->core->inep_regs[ep.number].diepctl.b.setd0pid = true;
    }
    else
    {
        data->core->outep_regs[ep.number].doepctl.b.stall = !!stall;
        if (!stall) data->core->outep_regs[ep.number].doepctl.b.setd0pid = true;
    }
}
void synopsysotg_set_address(const struct usb_instance* instance, uint8_t address)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    data->core->dregs.dcfg.b.devaddr = address;
}
void synopsysotg_unconfigure_ep(const struct usb_instance* instance, union usb_endpoint_number ep)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        synopsysotg_flush_in_endpoint(instance, ep.number);
        data->core->dregs.daintmsk.ep.in &= ~(1 << ep.number);
    }
    else
    {
        synopsysotg_flush_out_endpoint(instance, ep.number);
        data->core->dregs.daintmsk.ep.out &= ~(1 << ep.number);
    }
}
void synopsysotg_configure_ep(const struct usb_instance* instance, union usb_endpoint_number ep,
                              enum usb_endpoint_type type, int maxpacket)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    int txfifo = data->shared_txfifo ? 0 : ep.number;
    union synopsysotg_depctl depctl = { .b = { .usbactep = 1, .eptype = type, .mps = maxpacket,
                                               .txfnum = txfifo, .setd0pid = 1, .nextep = (ep.number + 1) & 0xf } };
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        data->core->inep_regs[ep.number].diepctl = depctl;
        data->core->dregs.daintmsk.ep.in |= (1 << ep.number);
    }
    else
    {
        data->core->outep_regs[ep.number].doepctl = depctl;
        data->core->dregs.daintmsk.ep.out |= (1 << ep.number);
    }
}
void synopsysotg_ep0_start_rx(const struct usb_instance* instance, bool non_setup, int len)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    struct synopsysotg_state* state = (struct synopsysotg_state*)instance->driver_state;
    if (data->use_dma) data->core->outep_regs[0].doepdma = instance->buffer;
    else state->endpoints[0].rxaddr = (uint32_t*)instance->buffer;
    union synopsysotg_dep0xfrsiz deptsiz = { .b = { .supcnt = 3, .pktcnt = !!non_setup, .xfersize = len } };
    data->core->outep_regs[0].doeptsiz.d32 = deptsiz.d32;
#ifdef TARGET_ipodnano5g
    if (data->use_dma)
    {
        if (wedge_fix_enable & (1u << 0)) invalidate_dcache_range((uint32_t)instance->buffer, len);
        else invalidate_dcache(instance->buffer, len);
    }
#else
    if (data->use_dma) invalidate_dcache(instance->buffer, len);
#endif
    union synopsysotg_depctl depctl = data->core->outep_regs[0].doepctl;
    depctl.b.epena = 1;
    depctl.b.cnak = !!non_setup;
    data->core->outep_regs[0].doepctl = depctl;
}
void synopsysotg_ep0_start_tx(const struct usb_instance* instance, const void* buf, int len)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    struct synopsysotg_state* state = (struct synopsysotg_state*)instance->driver_state;
#ifdef TARGET_ipodnano5g
    WEDGE_SNAP_TXFREE = wedge_txfifo_free(data, 0);
#endif
    if (len)
    {
#ifdef TARGET_ipodnano5g
        if (data->use_dma && (wedge_fix_enable & (1u << 1)))
        {
            uint32_t depth = data->txfifosize[0];
            uint32_t free = wedge_txfifo_free(data, 0);
            if (depth && free < depth) synopsysotg_flush_txfifo(instance, 0);
        }
#endif
        if (data->use_dma) data->core->inep_regs[0].diepdma = buf;
        else state->endpoints[0].txaddr = buf;
        union synopsysotg_dep0xfrsiz deptsiz = { .b = { .pktcnt = (len + 63) >> 6, .xfersize = len } };
        data->core->inep_regs[0].dieptsiz.d32 = deptsiz.d32;
    }
    else
    {
        union synopsysotg_dep0xfrsiz deptsiz = { .b = { .pktcnt = 1 } };
        data->core->inep_regs[0].dieptsiz.d32 = deptsiz.d32;
    }
#ifdef TARGET_ipodnano5g
    if (data->use_dma)
    {
        if (wedge_fix_enable & (1u << 0)) clean_dcache_range((uint32_t)buf, len);
        else clean_dcache(buf, len);
    }
#else
    if (data->use_dma) clean_dcache(buf, len);
#endif
    union synopsysotg_depctl depctl = data->core->inep_regs[0].diepctl;
    depctl.b.epena = 1;
    depctl.b.cnak = 1;
    data->core->inep_regs[0].diepctl = depctl;
    if (len && !data->use_dma)
    {
        if (data->shared_txfifo) synopsysotg_try_push(instance, 0);
        else data->core->dregs.diepempmsk.ep.in |= 1;
    }
}
static void synopsysotg_ep0_init(const struct usb_instance* instance)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    union synopsysotg_depctl depctl = { .b = { .usbactep = 1, .nextep = data->core->inep_regs[0].diepctl.b.nextep } };
    data->core->outep_regs[0].doepctl = depctl;
    data->core->inep_regs[0].diepctl = depctl;
}
void synopsysotg_irq(const struct usb_instance* instance)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    struct synopsysotg_state* state = (struct synopsysotg_state*)instance->driver_state;
#ifdef TARGET_ipodnano5g
    uint32_t wedge_t0 = USEC_TIMER;
#endif
    union synopsysotg_gintsts gintsts = data->core->gregs.gintsts;
#ifdef TARGET_ipodnano5g
    data->core->gregs.gintsts = gintsts;
#endif
    if (gintsts.b.usbreset)
    {
        data->core->dregs.dcfg.b.devaddr = 0;
        usb_handle_bus_reset(instance, 0);
#ifdef TARGET_ipodnano5g
        if (wedge_fix_enable & (1u << 3))
        { union synopsysotg_gintsts c = { .b = { .usbreset = 1 } }; data->core->gregs.gintsts = c; }
#endif
    }
    if (gintsts.b.enumdone)
    {
        usb_handle_bus_reset(instance, data->core->dregs.dsts.b.enumspd == 0);
        synopsysotg_ep0_init(instance);
#ifdef TARGET_ipodnano5g
        if (wedge_fix_enable & (1u << 3))
        { union synopsysotg_gintsts c = { .b = { .enumdone = 1 } }; data->core->gregs.gintsts = c; }
#endif
    }
    if (gintsts.b.rxstsqlvl && !data->use_dma)
    {
        union synopsysotg_grxfsts rxsts = data->core->gregs.grxstsp;
        int ep = rxsts.b.chnum;
        int words = (rxsts.b.bcnt + 3) >> 2;
        while (words--) *state->endpoints[ep].rxaddr++ = data->core->dfifo[0][0];
#ifdef TARGET_ipodnano5g
        if (wedge_fix_enable & (1u << 3))
        { union synopsysotg_gintsts c = { .b = { .rxstsqlvl = 1 } }; data->core->gregs.gintsts = c; }
#endif
    }
    if (gintsts.b.nptxfempty && data->core->gregs.gintmsk.b.nptxfempty)
    {
        data->core->gregs.gintmsk.b.nptxfempty = false;
        int ep;
        for (ep = 0; ep < 16; ep++) synopsysotg_try_push(instance, ep);
#ifdef TARGET_ipodnano5g
        if (wedge_fix_enable & (1u << 3))
        { union synopsysotg_gintsts c = { .b = { .nptxfempty = 1 } }; data->core->gregs.gintsts = c; }
#endif
    }
    if (gintsts.b.inepintr)
    {
        union synopsysotg_daint daint = data->core->dregs.daint;
        int ep;
        for (ep = 0; ep < 16; ep++)
            if (daint.ep.in & (1 << ep))
            {
                union synopsysotg_diepintn epints = data->core->inep_regs[ep].diepint;
                if (epints.b.emptyintr)
                {
                    union synopsysotg_depxfrsiz deptsiz = data->core->inep_regs[ep].dieptsiz;
                    if (!deptsiz.b.xfersize) data->core->dregs.diepempmsk.ep.in &= ~(1 << ep);
                    else
                    {
                        int left = (deptsiz.b.xfersize + 3) >> 2;
                        while (left)
                        {
                            int words = data->core->inep_regs[ep].dtxfsts.b.txfspcavail;
                            if (words > left) words = left;
                            if (!words) break;
                            left -= words;
                            while (words--) data->core->dfifo[ep][0] = *state->endpoints[ep].txaddr++;
                        }
                    }
                }
                union usb_endpoint_number epnum = { .direction = USB_ENDPOINT_DIRECTION_IN, .number = ep };
                int bytesleft = data->core->inep_regs[ep].dieptsiz.b.xfersize;
                data->core->inep_regs[ep].diepint = epints;
                if (epints.b.timeout) usb_handle_timeout(instance, epnum, bytesleft);
                if (epints.b.xfercompl) usb_handle_xfer_complete(instance, epnum, bytesleft);
            }
#ifdef TARGET_ipodnano5g
        if (wedge_fix_enable & (1u << 3))
        { union synopsysotg_gintsts c = { .b = { .inepintr = 1 } }; data->core->gregs.gintsts = c; }
#endif
    }
    if (gintsts.b.outepintr)
    {
        union synopsysotg_daint daint = data->core->dregs.daint;
        int ep;
        for (ep = 0; ep < 16; ep++)
            if (daint.ep.out & (1 << ep))
            {
                union synopsysotg_doepintn epints = data->core->outep_regs[ep].doepint;
                data->core->outep_regs[ep].doepint = epints;
                union usb_endpoint_number epnum = { .direction = USB_ENDPOINT_DIRECTION_OUT, .number = ep };
                if (epints.b.setup)
                {
#ifdef TARGET_ipodnano5g
                    if (data->use_dma)
                    {
                        if (wedge_fix_enable & (1u << 0))
                            invalidate_dcache_range((uint32_t)data->core->outep_regs[ep].doepdma, 8);
                        else invalidate_dcache((const void*)data->core->outep_regs[ep].doepdma, 8);
                    }
#else
                    if (data->use_dma) invalidate_dcache((const void*)data->core->inep_regs[ep].diepdma, 8);
#endif
                    synopsysotg_flush_in_endpoint(instance, ep);
#ifdef TARGET_ipodnano5g
                    if (!ep) synopsysotg_ep0_kill_stale_in(instance);
#endif
                    usb_handle_setup_received(instance, epnum);
                }
                else if (epints.b.xfercompl)
                {
#ifdef TARGET_ipodnano5g
                    int bytesleft = data->core->outep_regs[ep].doeptsiz.b.xfersize;
#else
                    int bytesleft = data->core->inep_regs[ep].dieptsiz.b.xfersize;
#endif
                    usb_handle_xfer_complete(instance, epnum, bytesleft);
                }
            }
#ifdef TARGET_ipodnano5g
        if (wedge_fix_enable & (1u << 3))
        { union synopsysotg_gintsts c = { .b = { .outepintr = 1 } }; data->core->gregs.gintsts = c; }
#endif
    }
#ifdef TARGET_ipodnano5g
    wedge_irq_hold_sample(wedge_t0, USEC_TIMER);
#else
    data->core->gregs.gintsts = gintsts;
#endif
}
void synopsysotg_init(const struct usb_instance* instance)
{
    int i;
#ifdef TARGET_ipodnano5g
    {
        for (i = 0; i < 14; i++) wedge_snap[i] = 0u;
        __asm volatile("mcr p15,0,%0,c7,c10,4" :: "r"(0));
    }
#endif
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    synopsysotg_target_disable_irq(instance);
    synopsysotg_target_enable_clocks(instance);
    union synopsysotg_pcgcctl pcgcctl = { .b = {} };
    data->core->pcgcctl = pcgcctl;
    union synopsysotg_gccfg gccfg = { .b = { .disablevbussensing = 1, .pwdn = 0 } };
    data->core->gregs.gccfg = gccfg;
    union synopsysotg_gusbcfg gusbcfg = { .b = { .force_dev = 1, .usbtrdtim = SYNOPSYSOTG_TURNAROUND } };
    if (data->phy_16bit) gusbcfg.b.phyif = 1;
    else if (data->phy_ulpi) gusbcfg.b.ulpi_utmi_sel = 1;
    else gusbcfg.b.physel  = 1;
    data->core->gregs.gusbcfg = gusbcfg;
    union synopsysotg_grstctl grstctl = { .b = { .csftrst = 1 } };
    udelay(100);
    while (!data->core->gregs.grstctl.b.ahbidle);
    data->core->gregs.grstctl = grstctl;
    while (data->core->gregs.grstctl.b.csftrst);
    while (!data->core->gregs.grstctl.b.ahbidle);
    union synopsysotg_dctl dctl = { .b = { .sftdiscon = 1 } };
    data->core->dregs.dctl = dctl;
    union synopsysotg_gahbcfg gahbcfg = { .b = { .dmaenable = data->use_dma, .hburstlen = SYNOPSYSOTG_AHB_BURST_LEN, .glblintrmsk = 1 } };
    if (data->disable_double_buffering)
    {
        gahbcfg.b.nptxfemplvl_txfemplvl = 1;
        gahbcfg.b.ptxfemplvl = 1;
    }
    data->core->gregs.gahbcfg = gahbcfg;
    data->core->gregs.gusbcfg = gusbcfg;
    gccfg.b.pwdn = 1;
    data->core->gregs.gccfg = gccfg;
    union synopsysotg_dcfg dcfg = { .b = { .nzstsouthshk = 1 } };
    data->core->dregs.dcfg = dcfg;
    if (data->use_dma)
    {
        union synopsysotg_dthrctl dthrctl = { .b = { .arb_park_en = 1, .rx_thr_en = 1, .iso_thr_en = 0, .non_iso_thr_en = 0,
                                                     .rx_thr_len = SYNOPSYSOTG_AHB_THRESHOLD } };
#ifdef TARGET_ipodnano5g
#else
        data->core->dregs.dthrctl = dthrctl;
#endif
    }
    int addr = data->fifosize;
    for (i = 0; i < 16; i++)
    {
        data->core->inep_regs[i].diepctl.b.nextep = (i + 1) & 0xf;
        int size = data->txfifosize[i];
        addr -= size;
        if (size)
        {
            union synopsysotg_txfsiz fsiz = { .b = { .startaddr = addr, .depth = size } };
            if (!i) data->core->gregs.dieptxf0_hnptxfsiz = fsiz;
            else data->core->gregs.dieptxf[i - 1] = fsiz;
        }
    }
    union synopsysotg_rxfsiz fsiz = { .b = { .depth = addr } };
    data->core->gregs.grxfsiz = fsiz;
    union synopsysotg_doepintn doepmsk =  { .b = { .xfercompl = 1, .setup = 1 } };
    data->core->dregs.doepmsk = doepmsk;
    union synopsysotg_diepintn diepmsk =  { .b = { .xfercompl = 1, .timeout = 1 } };
    data->core->dregs.diepmsk = diepmsk;
    data->core->dregs.diepempmsk.d32 = 0;
    union synopsysotg_daint daintmsk = { .ep = { .in = 0b0000000000000001, .out = 0b0000000000000001 } };
    data->core->dregs.daintmsk = daintmsk;
    union synopsysotg_gintmsk gintmsk =  { .b = { .usbreset = 1, .enumdone = 1, .outepintr = 1, .inepintr = 1 } };
    if (!data->use_dma) gintmsk.b.rxstsqlvl = 1;
    data->core->gregs.gintmsk = gintmsk;
    synopsysotg_flush_ints(instance);
    synopsysotg_target_clear_irq(instance);
    synopsysotg_target_enable_irq(instance);
    synopsysotg_target_run_mode(instance);
    dctl.b.sftdiscon = 0;
    data->core->dregs.dctl = dctl;
}
void synopsysotg_exit(const struct usb_instance* instance)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    union synopsysotg_dctl dctl = { .b = { .sftdiscon = 1 } };
    data->core->dregs.dctl = dctl;
    synopsysotg_target_disable_irq(instance);
    synopsysotg_target_disable_clocks(instance);
}
int synopsysotg_get_max_transfer_size(const struct usb_instance* data, union usb_endpoint_number ep)
{
    return 1023;
}
const struct usb_driver synopsysotg_driver =
{
    .init = synopsysotg_init,
    .exit = synopsysotg_exit,
    .ep0_start_rx = synopsysotg_ep0_start_rx,
    .ep0_start_tx = synopsysotg_ep0_start_tx,
    .start_rx = synopsysotg_start_rx,
    .start_tx = synopsysotg_start_tx,
    .get_stall = synopsysotg_get_stall,
    .set_stall = synopsysotg_set_stall,
    .set_address = synopsysotg_set_address,
    .configure_ep = synopsysotg_configure_ep,
    .unconfigure_ep = synopsysotg_unconfigure_ep,
    .get_max_transfer_size = synopsysotg_get_max_transfer_size,
};
