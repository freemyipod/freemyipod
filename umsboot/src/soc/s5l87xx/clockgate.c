#include "global.h"
#include "soc/s5l87xx/regs.h"
#include "sys/util.h"

bool clockgate_get_state(int gate)
{
    return !(PWRCON(gate >> 5) & (1 << (gate & 0x1f)));
}

void clockgate_enable(int gate, bool enable)
{
    enter_critical_section();
    if (enable) PWRCON(gate >> 5) &= ~(1 << (gate & 0x1f));
    else PWRCON(gate >> 5) |= 1 << (gate & 0x1f);
    leave_critical_section();
}
