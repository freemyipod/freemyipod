#ifndef __TARGET_UMSBOOT_IPODNANO2G_TARGET_H__
#define __TARGET_UMSBOOT_IPODNANO2G_TARGET_H__

#include "board/ipodnano2g/target.h"
#include "soc/s5l87xx/regs.h"

#define SRAM_SIZE 0x0002bffc
#define SDRAM_SIZE 0

#define CODE_REGION SRAM
#define BSS_REGION SRAM

#define S5L87XX_SYNOPSYSOTG_ENABLE

#define UMSBOOT_HAVE_CONSOLE

#define UMSBOOT_USB_DRIVER_HEADER "core/synopsysotg/synopsysotg.h"
#define UMSBOOT_USB_DRIVER synopsysotg_driver
#define UMSBOOT_USB_DRIVER_CONFIG_TYPE const struct synopsysotg_config
#define UMSBOOT_USB_DRIVER_CONFIG \
{ \
    .core = (struct synopsysotg_core_regs*)OTGBASE, \
    .phy_16bit = true, \
    .phy_ulpi = false, \
    .use_dma = true, \
    .shared_txfifo = true, \
    .disable_double_buffering = false, \
    .fifosize = 1024, \
    .txfifosize = { 0x200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, \
}
#define UMSBOOT_USB_DRIVER_STATE_TYPE struct synopsysotg_state
#define UMSBOOT_USB_DRIVER_STATE \
{ \
    .endpoints = { {}, {}, {} }, \
}
#define UMSBOOT_USB_VENDORID 0xffff
#define UMSBOOT_USB_VENDORSTRING { 'f', 'r', 'e', 'e', 'm', 'y', 'i', 'p', 'o', 'd', '.', 'o', 'r', 'g' }
#define UMSBOOT_USB_VENDORSTRING_LEN 16
#define UMSBOOT_USB_PRODUCTID 0x5562
#define UMSBOOT_USB_PRODUCTSTRING { 'U', 'M', 'S', 'b', 'o', 'o', 't' }
#define UMSBOOT_USB_PRODUCTSTRING_LEN 7
#define UMSBOOT_USB_DEVICEREVISION 2
#define UMSBOOT_USB_MAXCURRENT 100
#define UMSBOOT_ENDPOINT_OUT 2
#define UMSBOOT_ENDPOINT_IN 1

#define RAMDISK_BASEADDR 0x08000000
#define RAMDISK_SECTORSIZE 2048
#define RAMDISK_SECTORS 16384
#define RAMDISK_PTR_ADDR 0x2202bffc

#endif
