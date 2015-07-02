#include <assert.h>

#include <SDL2/SDL.h>

#include "context.h"
#include "cpu.h"
#include "ioregs.h"

void draw_line(context_t *ctx);

/*
 * Sets a new LCD state and requests an interrupt if appropriate.
 */
static inline void set_mode(context_t *ctx, gfx_state_t state)
{
    ctx->gfx.state = state;

    if (state < HBLANK_WAIT) {
        ctx->mem.io.STAT &= ~0x3;  // Clear bits 01
        ctx->mem.io.STAT |= state; // Set new state

        if (state < TRANSF && BIT_ISSET(ctx->mem.io.STAT, state + 3)) {
            cpu_irq(ctx, I_LCDC);
        }
    }
}

void oam(context_t *ctx)
{
    ctx->gfx.state = OAM_WAIT;

    if (ctx->mem.io.LY == ctx->mem.io.LYC)
    {
        // LY coincidence interrupt
        // The LCD just finished displaying a
        // line R_LY, which matches R_LYC.
        stat_lyc_set(&ctx->mem);
        if (stat_lyc_irq_enabled(&ctx->mem)) {
            // TODO: Why are we issuing LCDC here and above?
            cpu_irq(ctx, I_LCDC);
        }
    } else {
        stat_lyc_clear(&ctx->mem);
    }
}

void hblank(context_t *ctx)
{
    draw_line(ctx);
    ctx->mem.io.LY++;
    ctx->gfx.state = HBLANK_WAIT;
}

void vblank(context_t *ctx)
{
    gfx_t *gfx = &ctx->gfx;

    window_clear(&gfx->window);

    SDL_BlitSurface(gfx->sprites_bg, NULL, gfx->window.surface, NULL);
    SDL_BlitSurface(gfx->background, NULL, gfx->window.surface, NULL);
    SDL_BlitSurface(gfx->sprites_fg, NULL, gfx->window.surface, NULL);

    window_draw(&gfx->window);

    // Make overlays transparent again
    SDL_FillRect(gfx->sprites_bg, NULL, 0x00);
    SDL_FillRect(gfx->background, NULL, 0x00);
    SDL_FillRect(gfx->sprites_fg, NULL, 0x00);

    cpu_irq(ctx, I_VBLANK);

    gfx->state = VBLANK_WAIT;
    gfx->window_y = 0;
}

void oam_wait(context_t *ctx)
{
    if (ctx->gfx.cycles >= 80)
    {
        ctx->gfx.cycles -= 80;
        set_mode(ctx, TRANSF);
    }
}

void transf(context_t *ctx)
{
    if (ctx->gfx.cycles >= 172)
    {
        ctx->gfx.cycles -= 172;
        set_mode(ctx, HBLANK);
    }
}

void hblank_wait(context_t *ctx)
{
    if (ctx->gfx.cycles >= 204)
    {
        ctx->gfx.cycles -= 204;
        if (ctx->mem.io.LY == 144)
        {
            set_mode(ctx, VBLANK);
        }
        else
        {
            set_mode(ctx, OAM);
        }
    }
}

void vblank_wait(context_t *ctx)
{
    if (ctx->gfx.cycles >= 456)
    {
        ctx->gfx.cycles -= 456;
        
        if (ctx->mem.io.LY++ == 153)
        {
            ctx->mem.io.LY = 0;
            set_mode(ctx, OAM);

            if (ctx->stopflags & STOP_FRAME) {
                ctx->state = FRAME_STEPPED;
                ctx->stopflags &= ~STOP_FRAME;
            }
        }
    }
}
