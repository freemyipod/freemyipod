#include "global.h"
#include "soc/s5l87xx/irq.h"
#include "soc/s5l87xx/regs.h"
#include "cpu/arm/old/armutil.h"
#include "sys/util.h"

__attribute__((weak)) void unhandled_irq_handler()
{
    fault_handler();
}

__attribute__((weak,alias("unhandled_irq_handler"))) void ext0_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void ext1_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void ext2_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void eint_vbus_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void eintg_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_timer_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_wdt_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_unk1_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_unk2_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_unk3_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_dma_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_alarm_rtc_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_pri_rtc_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void reserved1_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_uart_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_usb_host_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_usb_func_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_lcdc_0_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_lcdc_1_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_calm_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_ata_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_uart0_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_spdif_out_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_ecc_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_sdci_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_lcd_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_wheel_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_iic_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void reserved2_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_mstick_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_adc_wakeup_irqhandler();
__attribute__((weak,alias("unhandled_irq_handler"))) void int_adc_irqhandler();

static void (* s5l8701_irq_vector[])(void) =
{
    ext0_irqhandler,
    ext1_irqhandler,
    ext2_irqhandler,
    eint_vbus_irqhandler,
    eintg_irqhandler,
    int_timer_irqhandler,
    int_wdt_irqhandler,
    int_unk1_irqhandler,
    int_unk2_irqhandler,
    int_unk3_irqhandler,
    int_dma_irqhandler,
    int_alarm_rtc_irqhandler,
    int_pri_rtc_irqhandler,
    reserved1_irqhandler,
    int_uart_irqhandler,
    int_usb_host_irqhandler,
    int_usb_func_irqhandler,
    int_lcdc_0_irqhandler,
    int_lcdc_1_irqhandler,
    int_calm_irqhandler,
    int_ata_irqhandler,
    int_uart0_irqhandler,
    int_spdif_out_irqhandler,
    int_ecc_irqhandler,
    int_sdci_irqhandler,
    int_lcd_irqhandler,
    int_wheel_irqhandler,
    int_iic_irqhandler,
    reserved2_irqhandler,
    int_mstick_irqhandler,
    int_adc_wakeup_irqhandler,
    int_adc_irqhandler,
};

__attribute__((interrupt("IRQ"))) void _irq_handler(void)
{
    int irq_no = INTOFFSET;
    s5l8701_irq_vector[irq_no]();
    SRCPND = (1 << irq_no);
    INTPND = INTPND;
}

void s5l87xx_irq_enable(int irq, bool state)
{
    enter_critical_section();
    if (state) INTMSK |= 1 << irq;
    else INTMSK &= ~(1 << irq);
    leave_critical_section();
}
