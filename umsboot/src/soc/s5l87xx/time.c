#include "global.h"
#include "sys/time.h"
#include "sys/util.h"
#include "soc/s5l87xx/regs.h"

unsigned int read_usec_timer()
{
#ifdef SOC_S5L8701
    discard(USEC_TIMER_H);
    return USEC_TIMER_L * 5;
#else
    return TECNT;
#endif
}
