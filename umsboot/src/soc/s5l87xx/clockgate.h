#ifndef __SOC_S5L87XX_CLOCKGATE_H__
#define __SOC_S5L87XX_CLOCKGATE_H__

#include "global.h"

extern bool clockgate_get_state(int gate);
extern void clockgate_enable(int gate, bool enable);

#endif
