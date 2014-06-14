#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#ifndef ASM_FILE
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
typedef __SIZE_TYPE__ size_t;
typedef uint32_t uintptr_t;
typedef int32_t intptr_t;
typedef uint8_t bool;
#define NULL ((void*)0)
#define true 1
#define false 0
#endif

#include "config.h"

#endif
