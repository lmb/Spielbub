#ifndef __TIMERS_H__
#define __TIMERS_H__

#include "context.h"

struct timers_opaque_t {
    unsigned int divider_cycles;
    unsigned int timer_cycles;
};

void timers_update(context_t *ctx, int cycles);

#endif//__TIMERS_H__
