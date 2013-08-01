#ifndef __SOC_S5L87XX_LCDIF_H__
#define __SOC_S5L87XX_LCDIF_H__

#include "global.h"
#include "interface/lcdif/lcdif.h"


struct __attribute__((packed,aligned(4))) sl87xx_lcdif_config
{
};

struct __attribute__((packed,aligned(4))) sl87xx_lcdif_state
{
};

extern const struct lcdif_driver s5l87xx_lcdif_driver;


#endif
