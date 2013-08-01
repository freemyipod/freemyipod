#include "global.h"
#include "soc/s5l87xx/irq.h"
#include "soc/s5l87xx/regs.h"
#include "cpu/arm/old/armutil.h"
#include "sys/util.h"

__attribute__((weak)) void unhandled_irq_handler()
{
    fault_handler();
}


__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq0_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq1_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq2_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq3_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq4_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq5_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq6_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq7_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_timer_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq9_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq10_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq11_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq12_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq13_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq14_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq15_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_dmac0_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_dmac1_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq18_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_usb_func_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq20_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq21_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq22_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_wheel_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq24_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq25_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq26_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq27_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq28_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_ata_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq30_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq31_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq32_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq33_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq34_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq35_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq36_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq37_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq38_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq39_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq40_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq41_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq42_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq43_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_mmc_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq45_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq46_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq47_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq48_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq49_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq50_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq51_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq52_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq53_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq54_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq55_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq56_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq57_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq58_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq59_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq60_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq61_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq62_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_irq63_irqhandler();

static void (* s5l87xx_irq_vector[])(void) =
{
    int_irq0_irqhandler,
    int_irq1_irqhandler,
    int_irq2_irqhandler,
    int_irq3_irqhandler,
    int_irq4_irqhandler,
    int_irq5_irqhandler,
    int_irq6_irqhandler,
    int_irq7_irqhandler,
    int_timer_irqhandler,
    int_irq9_irqhandler,
    int_irq10_irqhandler,
    int_irq11_irqhandler,
    int_irq12_irqhandler,
    int_irq13_irqhandler,
    int_irq14_irqhandler,
    int_irq15_irqhandler,
    int_dmac0_irqhandler,
    int_dmac1_irqhandler,
    int_irq18_irqhandler,
    int_usb_func_irqhandler,
    int_irq20_irqhandler,
    int_irq21_irqhandler,
    int_irq22_irqhandler,
    int_wheel_irqhandler,
    int_irq24_irqhandler,
    int_irq25_irqhandler,
    int_irq26_irqhandler,
    int_irq27_irqhandler,
    int_irq28_irqhandler,
    int_ata_irqhandler,
    int_irq30_irqhandler,
    int_irq31_irqhandler,
    int_irq32_irqhandler,
    int_irq33_irqhandler,
    int_irq34_irqhandler,
    int_irq35_irqhandler,
    int_irq36_irqhandler,
    int_irq37_irqhandler,
    int_irq38_irqhandler,
    int_irq39_irqhandler,
    int_irq40_irqhandler,
    int_irq41_irqhandler,
    int_irq42_irqhandler,
    int_irq43_irqhandler,
    int_mmc_irqhandler,
    int_irq45_irqhandler,
    int_irq46_irqhandler,
    int_irq47_irqhandler,
    int_irq48_irqhandler,
    int_irq49_irqhandler,
    int_irq50_irqhandler,
    int_irq51_irqhandler,
    int_irq52_irqhandler,
    int_irq53_irqhandler,
    int_irq54_irqhandler,
    int_irq55_irqhandler,
    int_irq56_irqhandler,
    int_irq57_irqhandler,
    int_irq58_irqhandler,
    int_irq59_irqhandler,
    int_irq60_irqhandler,
    int_irq61_irqhandler,
    int_irq62_irqhandler,
    int_irq63_irqhandler,
};

__attribute__((interrupt("IRQ"))) void _irq_handler(void)
{
    void* dummy __attribute__((unused)) = VIC0ADDRESS;
    dummy = VIC1ADDRESS;
    uint32_t irqs0 = VIC0IRQSTATUS;
    uint32_t irqs1 = VIC1IRQSTATUS;
    int irq;
    for (irq = 0; irqs0; irq++, irqs0 >>= 1)
        if (irqs0 & 1)
            s5l87xx_irq_vector[irq]();
    for (irq = 32; irqs1; irq++, irqs1 >>= 1)
        if (irqs1 & 1)
            s5l87xx_irq_vector[irq]();
    VIC0ADDRESS = NULL;
    VIC1ADDRESS = NULL;
}

void s5l87xx_irq_enable(int irq, bool state)
{
    if (state) VICINTENABLE(irq >> 5) = 1 << (irq & 0x1f);
    else VICINTENCLEAR(irq >> 5) = 1 << (irq & 0x1f);
}
