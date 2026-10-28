#include "global.h"
#ifdef TARGET_ipodnano5g
#include "global.h"
#include "synopsysotg.h"
#include "interrupt.h"
#include "clockgates.h"
#include "power.h"
#include "timer.h"

#include "target/ipodnano5g/s5l8730.h"


static const struct usb_instance* synopsysotg_instance;

/* iPod nano 5G (S5L8730) PHY bring-up = fw_disk usb_phy_power_on 0x080726c0 + 0x08039c5c:
   gates PHY (35) then OTG (2) + 5 ms (0x0803e318), PCGCCTL = 0 + 800 us (0x0803d834), PHY+0 = 0 (0x080726e2),
   PHY+4 = 0x10 | CHIPID 0x3d100008 b0 (0x080726f2, gate 46 for the chip-id block), PHY+0x3c = 2, PHY+0x38 = 0x22e0,
   10 us, PHY+0x1c = 4 (0x0803d178), DCTL = 2 (0x08039c62), PHY+8 = 9 + 10 us (0x08039c6a), PHY+0x40 low 11 bits
   0x347 (0x08039c80..cb4), PHY+0x44 low 12 bits 0xe3f (0x08039cc4..cf8), PHY+0x100 |= 1 + 10 us (0x08039d08),
   PHY+8 = 0 + 815 us (0x08039d14), PHY+0x1c = 6 before the core soft reset (0x08072720). Apple's PCGCCTL = 1 after the
   sequence is NOT copied (the 4G runs with 0). */
#define PHY(o) (*(volatile uint32_t*)(PHYBASE + (o)))

void synopsysotg_target_enable_clocks(const struct usb_instance* instance)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    uint32_t chipbit;
    clockgate_enable(CLOCKGATE_USB_2, true);    /* PHY first, then OTG (0x0803e318) */
    clockgate_enable(CLOCKGATE_USB_1, true);
    udelay(5000);
    data->core->pcgcctl.d32 = 0;
    udelay(800);
    clockgate_enable(CLOCKGATE_ECID, true);     /* chipinfo[2] = CHIPID+8 b0 (fw_disk:0x080409b4) */
    chipbit = CHIPID(8) & 1;
    clockgate_enable(CLOCKGATE_ECID, false);
    PHY(0x00) = 0;                              /* power up */
    PHY(0x04) = 0x10 | chipbit;                 /* PROVISIONAL: 0x10 | fuse bit; WTF writes 1  */
    PHY(0x3c) = 2;
    PHY(0x38) = 0x22e0;
    udelay(10);
    PHY(0x1c) = 4;
    data->core->dregs.dctl.d32 = 2;             /* soft disconnect inside the PHY reset */
    PHY(0x08) = 9;
    udelay(10);
    PHY(0x40) = (PHY(0x40) & ~0x7ffu) | 0x347;
    PHY(0x44) = (PHY(0x44) & ~0xfffu) | 0xe3f;
    PHY(0x100) |= 1;
    udelay(10);
    PHY(0x08) = 0;
    udelay(815);
    PHY(0x1c) = 6;                              /* 0x08072720: just before GRSTCTL.CSftRst in synopsysotg_init */
}

/* usb_phy_set_run_mode(1) 0x0807263c: PHY+0x30 &= ~4, PHY+0x1c = 0; called by synopsysotg_init right before the
   soft reconnect (the PCGCCTL 0/1 writes around it are not copied, see above). */
void synopsysotg_target_run_mode(const struct usb_instance* instance)
{
    (void)instance;
    PHY(0x30) &= ~4u;
    PHY(0x1c) = 0;
}

/* usb_phy_power_off 0x0807267e: PHY+0x1c = 4, PHY+0x34 &= ~3, PHY+0 = 0x1f, 10 us, PHY+8 = 1,
   10 us; gates off OTG then PHY (0x0803e318). */
void synopsysotg_target_disable_clocks(const struct usb_instance* instance)
{
    const struct synopsysotg_config* data = (const struct synopsysotg_config*)instance->driver_config;
    PHY(0x1c) = 4;
    PHY(0x34) &= ~3u;
    data->core->pcgcctl.d32 = 0;
    PHY(0x00) = 0x1f;                           /* power down */
    udelay(10);
    PHY(0x08) = 1;                              /* reset asserted */
    udelay(10);
    clockgate_enable(CLOCKGATE_USB_1, false);
    clockgate_enable(CLOCKGATE_USB_2, false);
}
#undef PHY

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

#else
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


void synopsysotg_target_run_mode(const struct usb_instance* instance)
{
    (void)instance;
}
#endif
