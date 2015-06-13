#include <stdbool.h>

#include "context.h"
#include "joypad.h"

#include "logging.h"
#include "meta.h"

static unsigned int current_instances = 0;

bool context_init_minimal(context_t *ctx)
{
    memset(ctx, 0, sizeof(context_t));

    cpu_init(&ctx->cpu);
    mem_init(&ctx->mem);

    if (!graphics_init(&ctx->gfx)) {
        return false;
    }

#if defined(DEBUG)
    ctx->logs = cb_init(LOG_NUM, LOG_LEN);
    if (ctx->logs == NULL) {
        return false;
    }

    ctx->traceback = cb_init(20, 2);
    if (ctx->traceback == NULL) {
        return false;
    }
#endif

    return true;
}

context_t* context_create(update_func_t func, void* context)
{
    if (current_instances++ != 0) {
        goto error;
    }

    context_t *ctx = malloc(sizeof(context_t));

    if (ctx == NULL) {
        return NULL;
    }

    if (SDL_Init(0) < 0)
    {
        goto error;
    }

    if (!context_init_minimal(ctx)) {
        goto error;
    }

    joypad_init(ctx);

    ctx->update_func = func;
    ctx->update_func_context = context;

    ctx->state = RUNNING;

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
        cb_destroy(ctx->traceback);
        set_init(&ctx->breakpoints);
#endif

        graphics_destroy(&ctx->gfx);

        --current_instances;
        SDL_Quit();

        free(ctx);
    }
}

bool context_load_rom(context_t *ctx, const char* filename)
{
    return mem_load_rom(&ctx->mem, filename);
}

bool context_run(context_t* ctx)
{
    SDL_Event event;
    unsigned int cycles_frame = 0;

    ctx->next_run = SDL_GetTicks() + TICKS_PER_FRAME;
    ctx->running = true;

    while (ctx->running)
    {
        while (ctx->state == RUNNING) {
            int cycles;

#if defined(DEBUG)
            cb_write(ctx->traceback, &ctx->cpu.PC);
#endif

            if (ctx->cpu.IME) {
                // Interrupt Master Enable
                cpu_interrupts(ctx);
            }

            if (ctx->cpu.halted) {
                cycles = 4;
            } else {
                cycles = cpu_run(ctx);
            }

            // Update graphics, timers, etc.
            timers_update(ctx, cycles);
            graphics_update(ctx, cycles);
            joypad_update(ctx);

#if defined(DEBUG)
            if (ctx->stopflags & STOP_STEP)
            {
                ctx->state = SINGLE_STEPPED;
                ctx->stopflags &= ~STOP_STEP;
            } else if (set_contains(&ctx->breakpoints, ctx->cpu.PC)) {
                ctx->state = BREAKPOINT;
            }
#endif

            cycles_frame += cycles;
            if (cycles_frame >= CYCLES_PER_FRAME) {
                cycles_frame -= CYCLES_PER_FRAME;
                break;
            }
        }

        while (SDL_PollEvent(&event))
        {
            if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP))
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    // Return if ESC is pressed.
                    return true;
                }
                
                // Update current joypad state
                joypad_update_state(ctx, &(event.key));
            }
            else if (event.type == SDL_QUIT)
            {
                return true;
            }
        }

        if (ctx->update_func != NULL) {
            ctx->update_func(ctx, ctx->update_func_context);
        }

        if (SDL_GetTicks() < ctx->next_run) {
            unsigned int delay_by;
            
            delay_by = ctx->next_run - SDL_GetTicks();
            SDL_Delay(delay_by);
        }

        // TODO: This does overflow at some point.
        ctx->next_run += TICKS_PER_FRAME;
    }

    return true;
}

void context_quit(context_t* ctx)
{
    ctx->running = false;
}

size_t context_decode_instruction(const context_t* ctx, uint16_t addr,
    char dst[], size_t len)
{
    return meta_parse(dst, len, &ctx->mem.map[addr]);
}

void context_resume_exec(context_t* ctx)
{
    ctx->state = RUNNING;
}

void context_pause_exec(context_t* ctx)
{
    ctx->state = STOPPED;
}

void context_single_step(context_t* ctx)
{
    ctx->state = RUNNING;
    ctx->stopflags |= STOP_STEP;
}

void context_frame_step(context_t* ctx)
{
    ctx->state = RUNNING;
    ctx->stopflags |= STOP_FRAME;
}

execution_state_t context_get_exec(context_t* ctx)
{
    return ctx->state;
}

bool context_add_breakpoint(context_t* ctx, uint16_t addr)
{
    return set_add(&ctx->breakpoints, addr);
}

void context_get_registers(const context_t* ctx, registers_t* regs)
{
    regs->AF = ctx->cpu.AF;
    regs->BC = ctx->cpu.BC;
    regs->DE = ctx->cpu.DE;
    regs->HL = ctx->cpu.HL;
    regs->SP = ctx->cpu.SP;
    regs->PC = ctx->cpu.PC;
}

uint16_t context_get_memory(const context_t* ctx, uint8_t buffer[],
    uint16_t addr, uint16_t len)
{
    len = 0xFFFF - len < addr ? 0xFFFF - addr : len;

    memcpy(buffer, &ctx->mem.map[addr], len);
    return len;
}

void context_reset_traceback(const context_t* ctx)
{
    cb_reset(ctx->traceback);
}

bool context_get_traceback(const context_t* ctx, uint16_t* value)
{
    return cb_read(ctx->traceback, (uint8_t*)value);
}
