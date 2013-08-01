#ifndef __LIB_SIMPLETEXTRENDERER_SIMPLETEXTRENDERER_H__
#define __LIB_SIMPLETEXTRENDERER_SIMPLETEXTRENDERER_H__

#include "global.h"
#include "interface/textrenderer/textrenderer.h"


struct __attribute__((packed,aligned(4))) simpletextrenderer_config
{
};

struct __attribute__((packed,aligned(4))) simpletextrenderer_state
{
};

extern const struct textrenderer_driver simpletextrenderer_driver;


#endif
