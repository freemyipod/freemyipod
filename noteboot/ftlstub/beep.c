#include "toolkit.h"
#include "timer.h"

void singlebeep(int tone, int time)
{
    TDPRE = 29;
    TDCON = 528;
    TDDATA0 = tone;
    TDDATA1 = tone << 1;
    TDCMD = 1;
    sleep(time);
    TDCMD = 2;
}

void beep(int count)
{
    int i;
    TDCMD = 2;
    while (1)
    {
        singlebeep(22, 1000000);
        sleep(300000);
        for (i = 0; i < count; i++)
        {
            singlebeep(22, 300000);
            sleep(300000);
        }
        sleep(2000000);
    }
}
