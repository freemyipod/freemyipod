#ifndef __INTERFACE_LCDIF_LCDIF_H__
#define __INTERFACE_LCDIF_LCDIF_H__

#include "global.h"


struct __attribute__((packed,aligned(4))) lcdif_instance;


struct __attribute__((packed,aligned(4))) lcdif_driver
{
    void (*init)(const struct lcdif_instance* instance);
    void (*send_cmd)(const struct lcdif_instance* instance, uint32_t cmd);
    void (*send_data)(const struct lcdif_instance* instance, uint32_t data);
    void (*send_bulk)(const struct lcdif_instance* instance, void* data, int count);
    void (*send_repeat)(const struct lcdif_instance* instance, uint32_t data, int words);
};

struct __attribute__((packed,aligned(4))) lcdif_instance
{
    const struct lcdif_driver* driver;
    const void* driver_config;
    void* driver_state;
};


#endif
