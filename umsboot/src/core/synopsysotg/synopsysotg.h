#ifndef __CORE_SYNOPSYSOTG_SYNOPSYSOTG_H__
#define __CORE_SYNOPSYSOTG_SYNOPSYSOTG_H__

#include "global.h"
#include "protocol/usb/usb.h"
#include "core/synopsysotg/regs.h"

struct __attribute__((packed,aligned(4))) synopsysotg_config
{
    volatile struct synopsysotg_core_regs* core;
    uint32_t phy_16bit : 1;
    uint32_t phy_ulpi : 1;
    uint32_t use_dma : 1;
    uint32_t shared_txfifo : 1;
    uint32_t disable_double_buffering : 1;
    uint32_t reserved0 : 3;
    uint8_t reserved1;
    uint16_t fifosize;
    uint16_t txfifosize[16];
};

struct __attribute__((packed,aligned(4))) synopsysotg_state
{
    int dummy[0];
    struct
    {
        uint32_t* rxaddr;
        const uint32_t* txaddr;
    } endpoints[];
};

extern const struct usb_driver synopsysotg_driver;

extern void synopsysotg_irq(const struct usb_instance* instance);
extern void synopsysotg_target_enable_clocks(const struct usb_instance* instance);
extern void synopsysotg_target_disable_clocks(const struct usb_instance* instance);
extern void synopsysotg_target_enable_irq(const struct usb_instance* instance);
extern void synopsysotg_target_disable_irq(const struct usb_instance* instance);
extern void synopsysotg_target_clear_irq(const struct usb_instance* instance);

#endif
