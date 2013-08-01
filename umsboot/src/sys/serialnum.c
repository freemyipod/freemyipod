#include "global.h"
#include "sys/serialnum.h"

uint32_t __attribute__((weak,alias("serialnum_chip"))) serialnum_board();

uint32_t __attribute__((weak,pure)) serialnum_chip()
{
    return 0xffffffff;
}
