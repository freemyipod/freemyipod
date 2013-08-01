#ifndef __SOC_S5L87XX_I2C_H__
#define __SOC_S5L87XX_I2C_H__

#include "global.h"
#include "protocol/i2c/i2c.h"

#ifndef ASM_FILE
struct __attribute__((packed,aligned(4))) s5l87xx_i2c_driver_config
{
    uint8_t index;
    uint8_t reserved[3];
};

struct __attribute__((packed,aligned(4))) s5l87xx_i2c_driver_state
{
};

extern const struct i2c_driver s5l87xx_i2c_driver;
#endif

#endif
