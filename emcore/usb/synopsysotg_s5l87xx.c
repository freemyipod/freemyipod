#include "global.h"
#include "synopsysotg.h"
#include "interrupt.h"
#include "clockgates.h"
#include "power.h"
#include "timer.h"

#ifdef TARGET_ipodnano2g
#include "target/ipodnano2g/s5l8701.h"
#endif
#if defined(TARGET_ipodnano3g) || defined(TARGET_ipodclassic)
#include "target/ipodnano3g/s5l8702.h"
#endif
#if defined(TARGET_ipodnano4g) || defined(TARGET_ipodtouch2g)
#include "target/ipodnano4g/s5l8720.h"
#endif


static const struct usb_instance* synopsysotg_instance;

void synopsysotg_target_enable_clocks(const struct usb_instance* instance)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    clockgate_enable(CLOCKGATE_USB_1, true);
    clockgate_enable(CLOCKGATE_USB_2, true);
    data->core->pcgcctl.d32 = 0;
    *((volatile uint32_t*)(PHYBASE + 0x00)) = 0;  /* PHY: Power up */
    udelay(10);
    *((volatile uint32_t*)(PHYBASE + 0x1c)) = 1;
    *((volatile uint32_t*)(PHYBASE + 0x44)) = 0xe3f;
    *((volatile uint32_t*)(PHYBASE + 0x08)) = 1;  /* PHY: Assert Software Reset */
    udelay(10);
    *((volatile uint32_t*)(PHYBASE + 0x08)) = 0;  /* PHY: Deassert Software Reset */
    udelay(10);
    *((volatile uint32_t*)(PHYBASE + 0x18)) = 0x600;
    *((volatile uint32_t*)(PHYBASE + 0x04)) = SYNOPSYSOTG_CLOCK;
    udelay(400);
}

void synopsysotg_target_disable_clocks(const struct usb_instance* instance)
{
    *((volatile uint32_t*)(PHYBASE + 0x00)) = 0xf;  /* PHY: Power down */
    udelay(10);
    *((volatile uint32_t*)(PHYBASE + 0x08)) = 7;  /* PHY: Assert Software Reset */
    udelay(10);
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    data->core->pcgcctl.d32 = 0;
    clockgate_enable(CLOCKGATE_USB_1, false);
    clockgate_enable(CLOCKGATE_USB_2, false);
}

void synopsysotg_target_enable_irq(const struct usb_instance* instance)
{
    synopsysotg_instance = instance;
    interrupt_enable(IRQ_USB_FUNC, true);
}

void synopsysotg_target_disable_irq(const struct usb_instance* instance)
{
    interrupt_enable(IRQ_USB_FUNC, false);
}

void synopsysotg_target_clear_irq(const struct usb_instance* instance)
{
}

void INT_USB_FUNC()
{
    synopsysotg_irq(synopsysotg_instance);
}

