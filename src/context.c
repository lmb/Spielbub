#include <stdbool.h>

#include "context.h"
#include "hardware.h"
#include "joypad.h"

#include "logging.h"

bool context_init_minimal(context_t *ctx)
{
    memset(ctx, 0, sizeof(context_t));

    cpu_init(&ctx->cpu);
    mem_init(&ctx->mem);

#if defined(DEBUG)
    ctx->logs = cb_init(LOG_NUM, LOG_LEN);

    if (!ctx->logs) {
        return false;
    }
#endif

    return true;
}

context_t* context_create(void)
{
    context_t *ctx = malloc(sizeof(context_t));

    if (!context_init_minimal(ctx)) {
        goto error;
    }

    if (!graphics_init(&ctx->gfx)) {
        goto error;
    }

    joypad_init(ctx);

    return ctx;

    error: {
        free(ctx);
        return NULL;
    }
}

void context_destroy(context_t *ctx)
{
    if (ctx != NULL) {
        mem_destroy(&ctx->mem);

#if defined(DEBUG)
        cb_destroy(ctx->logs);
#endif

        free(ctx);
    }
}

bool context_load_rom(context_t *ctx, const char* filename)
{
    return mem_load_rom(&ctx->mem, filename);
}

/* 
 * This is the main emulation loop.
 */
void context_run(context_t *ctx)
{
    SDL_Event event;
    unsigned int cycles_frame = 0;
    
    ctx->next_run = SDL_GetTicks() + (int)TICKS_PER_FRAME;
    ctx->state = RUNNING;

    for (;;)
    {
        while (cycles_frame < CYCLES_PER_FRAME && ctx->state != STOPPED) {
            int cycles;

            if (ctx->cpu.halted) {
                cycles = 4;
            } else {
                cycles = cpu_run(ctx);
            }

            cycles_frame += cycles;

            // Update graphics, timers, etc.
            timers_update(ctx, cycles);
            graphics_update(ctx, cycles);
            joypad_update(ctx);
            
            if (ctx->cpu.IME) {
                // Interrupt Master Enable
                cpu_interrupts(ctx);
            }
            
            if (ctx->state == SINGLE_STEPPING)
            {
                ctx->state = STOPPED;
            }
        }

        cycles_frame -= CYCLES_PER_FRAME;

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
                joypad_update_state(ctx, &(event.key));
            }
            else if (event.type == SDL_QUIT)
            {
                return;
            }
        }
        
        if (ctx->gfx.frame_rendered)
        {
            ctx->gfx.frame_rendered = false;
            
            // At this point a screen has been completely
            // drawn, the vbank period has elapsed, and the
            // GB is about to start drawing a new frame.
            // Limit the speed at which we draw to GB's 59.sth
            // fps.
            if (SDL_GetTicks() < ctx->next_run) {
                unsigned int delay_by;
                
                delay_by = ctx->next_run - SDL_GetTicks();
                SDL_Delay(delay_by);
            }
            
            // TODO: This does overflow at some point.
            ctx->next_run += (int)TICKS_PER_FRAME;
        }
    }
}
