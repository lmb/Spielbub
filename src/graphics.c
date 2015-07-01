#include <assert.h>

#include "context.h"

#include "ioregs.h"
#include "logging.h"
#include "graphics/tiles.h"

#define NUM(x) (sizeof x / sizeof x[0])
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static const SDL_Color sdl_palette[] = {
    {0xFF, 0x00, 0x00, 0x00}, // Transparent
    {0xCC, 0xCC, 0xCC, 0xFF}, // Light grey
    {0x77, 0x77, 0x77, 0xFF}, // Dark grey
    {0x00, 0x00, 0x00, 0xFF}, // Black
};

/* BIG MESS, you have been warned */

void draw_line();

SDL_Surface* create_surface() {
    SDL_Surface *surface = SDL_CreateRGBSurface(
        0, SCREEN_WIDTH, SCREEN_HEIGHT, 8,
        0, 0, 0, 0
    );

    if (surface == NULL) {
        return NULL;
    }

    assert(surface->format->palette != NULL);
    assert(SDL_SetPaletteColors(surface->format->palette, sdl_palette, 0,
        NUM(sdl_palette)) == 0);

    assert(SDL_SetColorKey(surface, SDL_TRUE, 0) == 0);

    SDL_FillRect(surface, NULL, 0);

    return surface;
}

bool graphics_init(gfx_t* gfx)
{
    memset(gfx, 0, sizeof *gfx);

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        return false;
    }

    if (!window_init(&gfx->window, "Spielbub", SCREEN_WIDTH, SCREEN_HEIGHT)) {
        return false;
    }

    gfx->background = create_surface();
    gfx->sprites_bg = create_surface();
    gfx->sprites_fg = create_surface();

    if (!gfx->background || !gfx->sprites_bg || ! gfx->sprites_fg) {
        goto error;
    }

    gfx->layers[0] = gfx->background;
    gfx->layers[1] = gfx->sprites_bg;
    gfx->layers[2] = gfx->sprites_fg;
    gfx->state = OAM;

    window_clear(&gfx->window);
    window_draw(&gfx->window);

    return true;

    error: {
        graphics_destroy(gfx);
        return false;
    }
}

void graphics_destroy(gfx_t *gfx)
{
    if (gfx != NULL) {
        window_destroy(&gfx->window);

        if (gfx->background != NULL) {
            SDL_FreeSurface(gfx->background);
            gfx->background = NULL;
        }

        if (gfx->sprites_bg != NULL) {
            SDL_FreeSurface(gfx->sprites_bg);
            gfx->sprites_bg = NULL;
        }

        if (gfx->sprites_fg != NULL) {
            SDL_FreeSurface(gfx->sprites_fg);
            gfx->sprites_fg = NULL;
        }

        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
}

bool graphics_lock(context_t *ctx)
{
    if (SDL_LockSurface(ctx->gfx.window.surface) < 0)
    {
        log_dbg(ctx, "Can not lock surface: %s", SDL_GetError());
        return false;
    }
    
    return true;
}

void graphics_unlock(context_t *ctx)
{
    SDL_UnlockSurface(ctx->gfx.window.surface);
}

void hblank(context_t*);
void vblank(context_t*);
void oam(context_t*);
void transf(context_t*);
void hblank_wait(context_t*);
void vblank_wait(context_t*);
void oam_wait(context_t*);

/*
 * Updates the screen.
 */
void graphics_update(context_t *ctx, int cycles)
{
    // context->state starts in OAM. As soon as a certain
    // amount of cycles is reached, state transitions
    // to TRANSF, etc.

    if (!BIT_ISSET(ctx->mem.io.LCDC, R_LCDC_ENABLED))
    {
        // LCD is disabled.
        ctx->mem.io.LY = 144;
        ctx->gfx.state = VBLANK_WAIT;
        ctx->gfx.cycles = 0;

        return;
    }

    ctx->gfx.cycles += cycles;

    switch (ctx->gfx.state)
    {
        case OAM:         oam(ctx);
        case OAM_WAIT:    oam_wait(ctx);    break;

        case TRANSF:      transf(ctx);      break;

        case HBLANK:      hblank(ctx);
        case HBLANK_WAIT: hblank_wait(ctx); break;

        case VBLANK:      vblank(ctx);
        case VBLANK_WAIT: vblank_wait(ctx); break;
    }
}

static palette_t
palette_decode(uint8_t raw_palette)
{
    /* Palettes are packed into an uint8_t and map color indexes to actual
     * colors. This is the layout:
     * Bits:        7 6 5 4 3 2 1 0
     * Color Index:  3   2   1   0
     *
     * gfx->palette holds the pre-mapped RGB pixel values for the actual colors.
     */
    palette_t palette;

    palette.colors[0] = ((raw_palette & (0x3 << (2*0))) >> (2*0));
    palette.colors[1] = ((raw_palette & (0x3 << (2*1))) >> (2*1));
    palette.colors[2] = ((raw_palette & (0x3 << (2*2))) >> (2*2));
    palette.colors[3] = ((raw_palette & (0x3 << (2*3))) >> (2*3));

    return palette;
}

static void
sprite_decode(sprite_t *sprite, const memory_oam_t* oam, palette_t high,
    palette_t low)
{
    sprite->y      = MAX(0, oam->data[0]  - SPRITE_HEIGHT);
    sprite->x      = MAX(0, oam->data[1]  - SPRITE_WIDTH);
    sprite->tile_y = MAX(0, SPRITE_HEIGHT - oam->data[0]);
    sprite->tile_x = MAX(0, SPRITE_WIDTH  - oam->data[1]);

    if (sprite->tile_y < TILE_HEIGHT) {
        sprite->tile_id = oam->data[2] & 0xFE;
    } else {
        sprite->tile_id = oam->data[2] | 0x01;
    }

    sprite->visible = false;
    if (sprite->tile_x < SPRITE_WIDTH && sprite->tile_y < SPRITE_HEIGHT) {
        sprite->visible = true;
    }

    sprite->tile_y %= TILE_HEIGHT;

    sprite->in_background = BIT_ISSET(oam->data[2], 7);
    sprite->flip_y        = BIT_ISSET(oam->data[2], 6);
    sprite->flip_x        = BIT_ISSET(oam->data[2], 5);
    sprite->palette       = BIT_ISSET(oam->data[2], 4) ? high : low;
}

void
draw_line(context_t *ctx) {
    gfx_t* gfx = &ctx->gfx;
    uint8_t lcdc = ctx->mem.io.LCDC;
    uint8_t screen_y = ctx->mem.io.LY;

    map_t src;
    dest_t dest;
    palette_t palette;

    if (!graphics_lock(ctx))
    {
        return;
    }
    
    // Background
    if (BIT_ISSET(lcdc, R_LCDC_BG_ENABLED))
    {
        palette = palette_decode(ctx->mem.io.BGP);

        map_init(
            &src, &ctx->mem,
            !BIT_ISSET(lcdc, R_LCDC_BG_TILE_MAP) ?
                &ctx->mem.gfx.map_low : &ctx->mem.gfx.map_high,
            ctx->mem.io.SCX, screen_y + ctx->mem.io.SCY
        );

        dest_init(
            &dest,
            gfx->background,
            0, screen_y, SCREEN_WIDTH
        );
        
        draw_tiles(&dest, &src, palette);
    }

    // Window
    uint8_t window_y = ctx->mem.io.WY;
    // TODO: What happens on underflow?
    uint8_t window_x = ctx->mem.io.WX - 7;
    
    if (BIT_ISSET(lcdc, R_LCDC_WINDOW_ENABLED) && screen_y >= window_y &&
        window_x < SCREEN_WIDTH)
    {
        palette = palette_decode(ctx->mem.io.BGP);

        map_init(
            &src, &ctx->mem,
            !BIT_ISSET(lcdc, R_LCDC_WINDOW_TILE_MAP) ?
                &ctx->mem.gfx.map_low : &ctx->mem.gfx.map_high,
            window_x, screen_y + gfx->window_y
        );

        dest_init(
            &dest,
            gfx->background,
            window_x, screen_y, SCREEN_WIDTH - window_x
        );

        draw_tiles(&dest, &src, palette);
        gfx->window_y += 1;
    }
    
    // Sprites
    if (BIT_ISSET(lcdc, R_LCDC_SPRITES_ENABLED))
    {
        size_t sprite_height = BIT_ISSET(lcdc, R_LCDC_SPRITES_LARGE) ? 16 : 8;
        palette_t spp_high, spp_low;

        spp_high = palette_decode(ctx->mem.io.SPP_HIGH);
        spp_low = palette_decode(ctx->mem.io.SPP_LOW);

        sprite_table_t sprites = { .length = 0 };
        
        for (size_t i = 0; i < MAX_SPRITES; i++)
        {
            sprite_t sprite;
            sprite_decode(&sprite, &ctx->mem.gfx.oam[i], spp_high, spp_low);

            if (!sprite.visible) {
                continue;
            }
            
            if (screen_y >= sprite.y && screen_y < sprite.y + sprite_height)
            {
                graphics_sprite_table_add(&sprites, &sprite);
            }
        }

        // <sprites> holds the 10 sprites with highest priority,
        // sorted by ascending x coordinate. Since sprites
        // with lower x coords write over tiles with higher x coords
        // we draw in reverse order.
        for (int i = sprites.length - 1; i >= 0; i--)
        {
            const sprite_t* sprite = &sprites.data[i];
            source_t src;
            dest_t dst;

            if (sprite->x >= SCREEN_WIDTH) {
                continue;
            }

            source_init(
                &src,
                &ctx->mem.gfx.tiles.data[sprite->tile_id],
                sprite->tile_x, sprite->tile_y + (screen_y - sprite->y)
            );

            dest_init(
                &dst,
                sprite->in_background ? gfx->sprites_bg : gfx->sprites_fg,
                sprite->x, screen_y, TILE_WIDTH
            );

            draw_tile(
                &dst,
                &src,
                sprite->palette
            );
        }
    }

    graphics_unlock(ctx);
}

void graphics_sprite_table_add(sprite_table_t *table, const sprite_t* sprite)
{
    size_t i;

    assert(table != NULL);
    assert(table->length <= NUM(table->data));

    if (table->length < NUM(table->data)) {
        table->data[table->length] = *sprite;
        i = table->length;
        table->length++;
    } else if (sprite->x < table->data[NUM(table->data) - 1].x) {
        table->data[NUM(table->data) - 1] = *sprite;
        i = NUM(table->data) - 1;
    } else {
        return;
    }

    while (i > 0 && table->data[i].x < table->data[i-1].x) {
        sprite_t temp    = table->data[i];
        table->data[i]   = table->data[i-1];
        table->data[i-1] = temp;
    }
}

void graphics_draw_tile(const context_t* ctx, window_t* window,
    uint16_t tile_id, size_t x, size_t y)
{
    palette_t palette = {
        {0, 1, 2, 3}
    };

    for (size_t tile_y = 0; tile_y < TILE_HEIGHT; tile_y++) {
        source_t src;
        dest_t dst;

        dest_init(&dst, window->surface, x, y + tile_y, window->surface->w - x);
        source_init(&src, &ctx->mem.gfx.tiles.data[tile_id], 0, tile_y);
        draw_tile(&dst, &src, palette);
    }
}

void graphics_toggle_debug(context_t* ctx, graphics_layer_t layer)
{
    static const SDL_Color debug[3][4] = {
        {
            {0xFF, 0x00, 0x00, 0x00}, // Transparent
            {0xD8, 0x8A, 0xDC, 0xFF}, // Light grey
            {0x9E, 0x51, 0xA3, 0xFF}, // Dark grey
            {0x4D, 0x00, 0x52, 0xFF}, // Black
        },
        {
            {0xFF, 0x00, 0x00, 0x00}, // Transparent
            {0x8A, 0xDC, 0x90, 0xFF}, // Light grey
            {0x51, 0xA3, 0x57, 0xFF}, // Dark grey
            {0x00, 0x52, 0x06, 0xFF}, // Black
        },
        {
            {0xFF, 0x00, 0x00, 0x00}, // Transparent
            {0x8A, 0xDC, 0xD8, 0xFF}, // Light grey
            {0x51, 0xA3, 0x9E, 0xFF}, // Dark grey
            {0x00, 0x52, 0x4D, 0xFF}, // Black
        }
    };

    const SDL_Color *palette;

    if (ctx->gfx.debug_flags & layer) {
        ctx->gfx.debug_flags &= ~layer;
        palette = sdl_palette;
    } else {
        ctx->gfx.debug_flags |= layer;
        palette = debug[layer - 1];
    }

    assert(SDL_SetPaletteColors(ctx->gfx.layers[layer - 1]->format->palette,
        palette, 0, 4) == 0);
}

bool graphics_get_debug(const context_t* ctx, graphics_layer_t layer)
{
    return ctx->gfx.debug_flags & layer;
}
