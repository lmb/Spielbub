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

void oam(context_t *ctx) {
    ctx->gfx.state = OAM_WAIT;

    if (ctx->mem.io.LY == ctx->mem.io.LYC)
    {
        // LY coincidence interrupt
        // The LCD just finished displaying a
        // line R_LY, which matches R_LYC.
        BIT_SET(ctx->mem.io.STAT, R_STAT_LY_COINCIDENCE);
        if (BIT_ISSET(ctx->mem.io.STAT, R_STAT_LYC_ENABLE)) {
            // TODO: Why are we issuing LCDC here and above?
            cpu_irq(ctx, I_LCDC);
        }
    } else {
        BIT_RESET(ctx->mem.io.STAT, R_STAT_LY_COINCIDENCE);
    }
}

void hblank(context_t *ctx) {
    draw_line(ctx);
    ctx->mem.io.LY++;
    ctx->gfx.state = HBLANK_WAIT;
}

void vblank(context_t *ctx) {
    gfx_t *gfx = &ctx->gfx;

    SDL_FillRect(gfx->window.surface, NULL, gfx->screen_white);
    SDL_BlitSurface(gfx->sprites_bg, NULL, gfx->window.surface, NULL);
    SDL_BlitSurface(gfx->background, NULL, gfx->window.surface, NULL);
    SDL_BlitSurface(gfx->sprites_fg, NULL, gfx->window.surface, NULL);

    window_draw(&gfx->window);

    // Make overlays transparent again
    SDL_FillRect(gfx->sprites_bg, NULL, gfx->palette.colors[0]);
    SDL_FillRect(gfx->background, NULL, gfx->palette.colors[0]);
    SDL_FillRect(gfx->sprites_fg, NULL, gfx->palette.colors[0]);

    cpu_irq(ctx, I_VBLANK);

    gfx->state = VBLANK_WAIT;
    gfx->window_y = 0;
}

void oam_wait(context_t *ctx) {
    if (ctx->gfx.cycles >= 80)
    {
        ctx->gfx.cycles -= 80;
        set_mode(ctx, TRANSF);
    }
}

void transf(context_t *ctx) {
    if (ctx->gfx.cycles >= 172)
    {
        ctx->gfx.cycles -= 172;
        set_mode(ctx, HBLANK);
    }
}

void hblank_wait(context_t *ctx) {
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

void vblank_wait(context_t *ctx) {
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
