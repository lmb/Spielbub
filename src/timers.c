#include "context.h"

#include "ioregs.h"

#define DIVIDER_FREQUENCY (16384)
#define DIVIDER_CYCLES (CLOCKSPEED / DIVIDER_FREQUENCY)

unsigned int const timer_cycles[0x4] = {
    CLOCKSPEED / 4096,  CLOCKSPEED / 262144,
    CLOCKSPEED / 65536, CLOCKSPEED / 16384
};

void timers_update(context_t *ctx, int cycles)
{
    timers_t *timers = &ctx->timers;
    
    // Divider
    timers->divider_cycles += cycles;
    if (timers->divider_cycles >= DIVIDER_CYCLES)
    {
        timers->divider_cycles -= DIVIDER_CYCLES;
        ctx->mem.io.DIV += 1;
    }
    
    // Timers
    if (tac_enabled(&ctx->mem))
    {
        timers->timer_cycles += cycles;
        
        int timer_type = tac_timer_type(&ctx->mem);
        if (timers->timer_cycles >= timer_cycles[timer_type])
        {
            timers->timer_cycles -= timer_cycles[timer_type];

            ctx->mem.io.TIMA += 1;

            if (ctx->mem.io.TIMA == 0)
            {
                ctx->mem.io.TIMA = ctx->mem.io.TMA;
                cpu_irq(ctx, I_TIMER);
            }
        }
    }
    else
    {
        // TODO: Is is correct to reset timer cycles if they are
        // disabled?
        timers->timer_cycles = 0;
    }
}
