#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#ifndef ASM_FILE
#include <stdint.h>
#include <stddef.h>
#ifndef NULL
#define NULL ((void*)0)
#endif
#ifndef bool
#define bool uint8_t
#define true 1
#define false 0
#endif
#ifndef size_t
#define size_t uint32_t
#endif
#endif

#include "config.h"

#endif
