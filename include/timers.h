#ifndef __TIMERS_H__
#define __TIMERS_H__

#include "spielbub.h"

typedef struct timers {
    unsigned int divider_cycles;
    unsigned int timer_cycles;
} timers_t;

void timers_update(context_t *ctx, int cycles);

#endif//__TIMERS_H__
