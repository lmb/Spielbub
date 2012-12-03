#include <stdbool.h>

#include "context.h"
#include "cpu.h"
#include "graphics.h"
#include "hardware.h"
#include "timers.h"
#include "joypad.h"
#include "ipc.h"
#include "memory.h"

#include "logging.h"

/*
 * Allocates a context_t structure's members.
 */
bool context_create(context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    
    // TODO: Check alloc
    ctx->cpu = malloc(sizeof(cpu_t));
    memset(ctx->cpu, 0, sizeof(cpu_t));
    
    ctx->ipc = malloc(sizeof(ipc_t));
    memset(ctx->ipc, 0, sizeof(ipc_t));
    
    ctx->gfx = malloc(sizeof(gfx_t));
    memset(ctx->gfx, 0, sizeof(gfx_t));
    
    ctx->mem = malloc(sizeof(memory_t));
    memset(ctx->mem, 0, sizeof(memory_t));
    
    return true;
}

/*
 * Initialize context.
 */
bool context_init(context_t* ctx)
{
    if (!context_create(ctx))
    {
        printf("Failed context_create\n");
        return false;
    }
    
    cpu_init(ctx->cpu);
    
    if (!graphics_init(ctx->gfx))
    {
        printf("Failed graphics_init\n");
        return false;
    }
    
    // TODO: Check return code
    if (!ipc_init(ctx->ipc))
    {
        printf("Failed ipc_init\n");
        return false;
    }
    
    mem_init(ctx->mem);
    joypad_init(ctx);

    return true;
}

void context_destroy(context_t *ctx)
{
    if (ctx != NULL)
    {
        if (ctx->cpu != NULL)
            free(ctx->cpu);
        
        if (ctx->ipc != NULL)
        {
            ipc_destroy(ctx->ipc);
            free(ctx->ipc);
        }
        
        if (ctx->gfx != NULL)
        {
            free(ctx->gfx);
        }
        
        if (ctx->mem)
        {
            mem_destroy(ctx->mem);
            free(ctx->mem);
        }
    }
}

/*
 * This is the main emulation loop.
 */
void run(context_t *context)
{
    SDL_Event event;
    
    context->next_run = SDL_GetTicks() + (int)TICKS_PER_FRAME;

    for (;;)
    {
        do {
            int cycles;
            
            if (context->cpu->halted)
                // ???
                cycles = 4;
            else
                cycles = cpu_run(context);
            
            // Update the global cycle count
            context->cpu->cycles += cycles;
            
            // Update graphics, timers, etc.
            timers_update(context, cycles);
            graphics_update(context, cycles);
            ipc_update(context->ipc);
            joypad_update(context);
            
            if (context->cpu->IME) // Interrupt Master Enable
                cpu_interrupts(context);
            
        } while(context->cpu->cycles < CYCLES_PER_FRAME);
        
        // We might overrun our budget by a few cycles.
        context->cpu->cycles -= (int)CYCLES_PER_FRAME;

        while (SDL_PollEvent(&event))
        {
            if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP))
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    // Return if ESC is pressed.
                    return;
                }
                
                // Update current joypad state
                joypad_update_state(context, &(event.key));
            }
            else if (event.type == SDL_QUIT)
            {
                return;
            }
        }
        
        if (SDL_GetTicks() < context->next_run) {
            unsigned int delay_by;
            
            delay_by = context->next_run - SDL_GetTicks();
            SDL_Delay(delay_by);
        }
        
        // TODO: This does overflow at some point.
        context->next_run += (int)TICKS_PER_FRAME;
    }
}
