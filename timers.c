#include "timers.h"
#include "cpu.h"
#include "hardware.h"
#include "context.h"
#include "ioregs.h"
#include "memory.h"

int const timer_cycles[0x4] = {
    CLOCKSPEED / 4096,  CLOCKSPEED / 262144,
    CLOCKSPEED / 65536, CLOCKSPEED / 16384
};

void timers_update(context_t *ctx, int cycles)
{
    uint8_t r_tac = mem_read(ctx->mem, R_TAC);

    if (r_tac & R_TAC_ENABLED) // Timers are enabled
    {
        ctx->timer_cycles -= cycles;

        if (ctx->timer_cycles <= timer_cycles[r_tac & R_TAC_TYPE])
        {
            // Every certain amount of clock cycles (depending on timer frequency
            // selected) R_TIMA is increased by one. If R_TIMA overflows,
            // R_TMA is written to R_TIMA and an interrupt is requested.
            uint8_t *r_tima = mem_address(ctx->mem, R_TIMA);
            ctx->timer_cycles = 0;

            if (*r_tima == 0xFF)
            {
                *r_tima = mem_read(ctx->mem, R_TMA);
                cpu_irq(ctx, I_TIMER);
            }
            else
            {
                (*r_tima)++;
            }
        }
    }
}
