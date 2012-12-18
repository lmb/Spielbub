#include "timers.h"
#include "cpu.h"
#include "hardware.h"
#include "context.h"
#include "ioregs.h"
#include "memory.h"

#define DIVIDER_FREQUENCY (16384)
#define DIVIDER_CYCLES (CLOCKSPEED / DIVIDER_FREQUENCY)

unsigned int const timer_cycles[0x4] = {
    CLOCKSPEED / 4096,  CLOCKSPEED / 262144,
    CLOCKSPEED / 65536, CLOCKSPEED / 16384
};

void timers_update(context_t *ctx, int cycles)
{
    timers_t *timers = ctx->timers;
    
    // Divider
    timers->divider_cycles += cycles;
    if (timers->divider_cycles >= DIVIDER_CYCLES)
    {
        timers->divider_cycles -= DIVIDER_CYCLES;
        uint8_t *r_div = mem_address(ctx->mem, R_DIV);
        *r_div += 1;
    }
    
    // Timers
    uint8_t r_tac = mem_read(ctx->mem, R_TAC);
    if (r_tac & R_TAC_ENABLED)
    {
        timers->timer_cycles += cycles;
        
        int const timer_type = r_tac & R_TAC_TYPE;
        if (timers->timer_cycles >= timer_cycles[timer_type])
        {
            timers->timer_cycles -= timer_cycles[timer_type];
            
            uint8_t *r_tima = mem_address(ctx->mem, R_TIMA);
            *r_tima += 1;
            
            if (*r_tima == 0)
            {
                *r_tima = mem_read(ctx->mem, R_TMA);
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
