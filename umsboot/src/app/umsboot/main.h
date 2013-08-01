#ifndef __APP_UMSBOOT_MAIN_H__
#define __APP_UMSBOOT_MAIN_H__

#include "global.h"

#define RAMDISK ((uint8_t(*)[RAMDISK_SECTORSIZE])(RAMDISK_BASEADDR))
#define RAMDISK_PTR (*((uint8_t**)(RAMDISK_PTR_ADDR)))

extern void umsboot_notify_ejected();

#endif
