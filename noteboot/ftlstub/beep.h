#ifndef __BEEP_H__
#define __BEEP_H__

#include "toolkit.h"

void singlebeep(int tone, int time);
void beep(int count) __attribute__((noreturn));

#endif
