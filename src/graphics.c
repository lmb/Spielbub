#include <assert.h>

#include "context.h"

#include "ioregs.h"
#include "logging.h"

#define NUM(x) (sizeof x / sizeof x[0])

typedef struct tile {
    uint8_t lines[TILE_HEIGHT][2];
} tile_t;

typedef struct dest {
    pixel_t* data;
    size_t remaining;
} dest_t;

typedef struct source {
    const tile_t* tile;
    size_t x, y;
} source_t;

typedef struct map {
    const uint8_t (*ids)[MAP_COLUMNS];
    bool signed_ids;

    const tile_t* tiles;

    size_t col;
    size_t tile_x, tile_y;
} map_t;

/* BIG MESS, you have been warned */

static void draw_line();

/*
 * Sets a new LCD state and requests an
 * interrupt if appropriate.
 */
static void set_mode(context_t *ctx, gfx_state_t state)
{
    ctx->gfx.state = state;

    ctx->mem.io.STAT &= ~0x3;  // Clear bits 01
    ctx->mem.io.STAT |= state; // Set new stae

    if (state < 3 && BIT_ISSET(ctx->mem.io.STAT, state + 3))
        cpu_irq(ctx, I_LCDC);
}

static SDL_Surface* surface_create(int w, int h)
{
    return SDL_CreateRGBSurface(
        0, w, h, 16,
        0x0F00, 0x00F0, 0x000F, 0xF000
    );
}

static void window_destroy(window_t* window);

static bool window_init(window_t* window, const char name[], int w, int h)
{
    window->window = SDL_CreateWindow(
        name,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w, h,
        0
    );

    if (window->window == NULL) {
        goto error;
    }

    window->renderer = SDL_CreateRenderer(window->window, -1, 0);

    if (window->renderer == NULL) {
        goto error;
    }

    window->texture = SDL_CreateTexture(
        window->renderer,
        PIXEL_FORMAT,
        SDL_TEXTUREACCESS_STREAMING,
        w, h
    );

    if (window->texture == NULL) {
        goto error;
    }

    window->surface = surface_create(w, h);

    if (window->surface == NULL) {
        goto error;
    }

    return true;

    error: {
        window_destroy(window);
        return false;
    }
}

void window_destroy(window_t* window)
{
    if (window->renderer != NULL) {
        SDL_DestroyRenderer(window->renderer);
        window->renderer = NULL;
    }

    if (window->window != NULL) {
        SDL_DestroyWindow(window->window);
        window->window = NULL;
    }

    if (window->texture != NULL) {
        SDL_DestroyTexture(window->texture);
        window->texture = NULL;
    }

    if (window->surface != NULL) {
        SDL_FreeSurface(window->surface);
        window->surface = NULL;
    }
}

void graphics_draw_window(window_t* window)
{
    SDL_UpdateTexture(window->texture, NULL,
        window->surface->pixels, window->surface->pitch);
    SDL_RenderClear(window->renderer);
    SDL_RenderCopy(window->renderer, window->texture, NULL, NULL);
    SDL_RenderPresent(window->renderer);
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

#define INIT_SURFACE(x) do { \
        x = surface_create(SCREEN_WIDTH, SCREEN_HEIGHT); \
        if (x == NULL) { \
            goto error; \
        } \
    } while(0)
    INIT_SURFACE(gfx->background);
    INIT_SURFACE(gfx->sprites_bg);
    INIT_SURFACE(gfx->sprites_fg);
#undef INIT_SURFACE

    gfx->palette.colors[0] =
        SDL_MapRGBA(gfx->background->format, 0x00, 0x00, 0x00, 0x00);
    gfx->palette.colors[1] =
        SDL_MapRGB(gfx->background->format, 0xCC, 0xCC, 0xCC);
    gfx->palette.colors[2] =
        SDL_MapRGB(gfx->background->format, 0x77, 0x77, 0x77);
    gfx->palette.colors[3] =
        SDL_MapRGB(gfx->background->format, 0x00, 0x00, 0x00);

    for (size_t i = 0; i < NUM(gfx->debug_palettes); i++) {
        gfx->debug_palettes[i].colors[1] =
            SDL_MapRGB(gfx->background->format, 0xCC, 0xCC, 0xCC);
        gfx->debug_palettes[i].colors[2] =
            SDL_MapRGB(gfx->background->format, 0x77, 0x77, 0x77);
        gfx->debug_palettes[i].colors[3] =
            SDL_MapRGB(gfx->background->format, 0x00, 0x00, 0x00);
    }

    gfx->debug_palettes[0].colors[0] =
        SDL_MapRGBA(gfx->background->format, 0xFF, 0x14, 0x93, 0x88);
    gfx->debug_palettes[1].colors[0] =
        SDL_MapRGBA(gfx->background->format, 0x32, 0xCD, 0x32, 0x88);
    gfx->debug_palettes[2].colors[0] =
        SDL_MapRGBA(gfx->background->format, 0x00, 0xF9, 0xF9, 0x88);

    gfx->screen_white =
        SDL_MapRGBA(gfx->background->format, 0xFF, 0xFF, 0xFF, 0xFF);
    gfx->state = OAM;

    SDL_FillRect(gfx->window.surface, NULL, gfx->screen_white);
    graphics_draw_window(&gfx->window);

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

window_t* graphics_create_window(const char name[], int w, int h)
{
    window_t* window = malloc(sizeof *window);

    if (window == NULL) {
        return NULL;
    }

    if (!window_init(window, name, w, h)) {
        return NULL;
    }

    return window;
}

void graphics_free_window(window_t* window)
{
    if (window != NULL) {
        window_destroy(window);
        free(window);
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

/*
 * Updates the screen.
 */
void graphics_update(context_t *ctx, int cycles)
{
    // This works a bit like a very crude state machine.
    // context->state starts in OAM. As soon as a certain
    // amount of cycles is reached, state transitions
    // to TRANSF, etc.

    uint8_t* r_stat = &ctx->mem.io.STAT;

    if (!BIT_ISSET(ctx->mem.io.LCDC, R_LCDC_ENABLED))
    {
        // LCD is disabled.
        ctx->mem.io.LY = 144;
        ctx->gfx.state = VBLANK_WAIT;
        ctx->gfx.cycles = 0;

        return;
    }

    gfx_t *gfx = &ctx->gfx;

    gfx->cycles += cycles;
    switch (gfx->state)
    {
        case OAM:
            gfx->state = OAM_WAIT;
            if (ctx->mem.io.LY == ctx->mem.io.LYC)
            {
                // LY coincidence interrupt
                // The LCD just finished displaying a
                // line R_LY, which matches R_LYC.
                BIT_SET(*r_stat, R_STAT_LY_COINCIDENCE);
                if (BIT_ISSET(*r_stat, R_STAT_LYC_ENABLE)) {
                    cpu_irq(ctx, I_LCDC);
                }
            } else {
                BIT_RESET(*r_stat, R_STAT_LY_COINCIDENCE);
            }

        case OAM_WAIT:
            if (gfx->cycles >= 80)
            {
                gfx->cycles -= 80;
                set_mode(ctx, TRANSF);
            }
            break;

        case TRANSF:
            if (gfx->cycles >= 172)
            {
                gfx->cycles -= 172;
                set_mode(ctx, HBLANK);
            }
            break;

        case HBLANK:
            draw_line(ctx);
            ctx->mem.io.LY++;
            gfx->state = HBLANK_WAIT;

        case HBLANK_WAIT:
            if (gfx->cycles >= 204)
            {
                gfx->cycles -= 204;
                if (ctx->mem.io.LY == 144)
                {
                    set_mode(ctx, VBLANK);
                }
                else
                {
                    set_mode(ctx, OAM);
                }
            }
            break;

        case VBLANK:
        {
            SDL_FillRect(gfx->window.surface, NULL, gfx->screen_white);
            SDL_BlitSurface(gfx->sprites_bg, NULL, gfx->window.surface, NULL);
            SDL_BlitSurface(gfx->background, NULL, gfx->window.surface, NULL);
            SDL_BlitSurface(gfx->sprites_fg, NULL, gfx->window.surface, NULL);

            graphics_draw_window(&gfx->window);
            
            // Make overlays transparent again
            SDL_FillRect(gfx->sprites_bg, NULL, gfx->palette.colors[0]);
            SDL_FillRect(gfx->background, NULL, gfx->palette.colors[0]);
            SDL_FillRect(gfx->sprites_fg, NULL, gfx->palette.colors[0]);
            
            cpu_irq(ctx, I_VBLANK);
            
            gfx->state = VBLANK_WAIT;
            gfx->window_y = 0;
        }

        case VBLANK_WAIT:
            if (gfx->cycles >= 456)
            {
                gfx->cycles -= 456;
                
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
            break;
    }
}

static void
dest_init(dest_t* dst, pixel_t* pixels, size_t x, size_t y,
    size_t w)
{
    if (x >= w) {
        assert(x < w);
    }

    dst->data = pixels + (y * w) + x;
    dst->remaining = w - x;
}

static void
source_init(source_t *src, const tile_t* tile, size_t x, size_t y)
{
    src->tile = tile;
    src->x = x % TILE_WIDTH;
    src->y = y % TILE_HEIGHT;
}

static void
map_init(map_t* map, const memory_t* mem, const tile_map_t* ids, size_t x,
    size_t y)
{
    x %= MAP_WIDTH;
    y %= MAP_HEIGHT;

    size_t row = y / TILE_HEIGHT;

    map->signed_ids = !BIT_ISSET(mem->io.LCDC, R_LCDC_TILE_DATA);

    map->ids   = &(*ids)[row];
    map->tiles = (tile_t*)mem->gfx.tiles;

    map->col    = x / TILE_WIDTH;
    map->tile_y = y % TILE_HEIGHT;
    map->tile_x = x % TILE_WIDTH;
}

static void
map_next(map_t* map, source_t* src)
{
    uint16_t id = (*map->ids)[map->col];

    if (map->signed_ids) {
        id = 256 + (int8_t)id;
    }

    assert(id < MAX_TILES);

    map->col = (map->col + 1) % MAP_COLUMNS;

    src->tile = &map->tiles[id];
    src->x = map->tile_x;
    src->y = map->tile_y;

    // Only the first tile can have a non-zero x offset
    map->tile_x = 0;
}

static void
palette_decode(palette_t* restrict palette,
    const palette_t* restrict base_palette, uint8_t raw_palette)
{
    /* Palettes are packed into an uint8_t and map color indexes to actual
     * colors. This is the layout:
     * Bits:        7 6 5 4 3 2 1 0
     * Color Index:  3   2   1   0
     *
     * gfx->palette holds the pre-mapped RGB pixel values for the actual colors.
     */
#define DECODE(col) \
    base_palette->colors[((raw_palette & (0x3 << (col*2))) >> (col*2))]

    palette->colors[0] = DECODE(0);
    palette->colors[1] = DECODE(1);
    palette->colors[2] = DECODE(2);
    palette->colors[3] = DECODE(3);
#undef DECODE
}

static void
sprite_decode(sprite_t *sprite, const uint8_t *data)
{
#define MAX(a, b) ((a) > (b) ? (a) : (b))
    sprite->y      = MAX(0, data[0] - SPRITE_HEIGHT);
    sprite->x      = MAX(0, data[1] - SPRITE_WIDTH);
    sprite->tile_y = MAX(0, SPRITE_HEIGHT - data[0]);
    sprite->tile_x = MAX(0, SPRITE_WIDTH - data[1]);

    sprite->tile_id = data[2];
    sprite->visible = sprite->tile_x < SPRITE_WIDTH
        && sprite->tile_y < SPRITE_HEIGHT;

    sprite->in_background = BIT_ISSET(data[2], 7);
    sprite->flip_y = BIT_ISSET(data[2], 6);
    sprite->flip_x = BIT_ISSET(data[2], 5);
    sprite->palette = BIT_ISSET(data[2], 4) ? PALETTE_HIGH : PALETTE_LOW;
#undef MAX
}

static void
draw_tile(dest_t* restrict dst, const source_t* restrict src,
    const palette_t* restrict palette)
{
    /* Tiles are 8x8 pixels, with every pixel taking up two bits (for four
     * colors). The pixel bits are not continuous in memory, but split over
     * two bytes.
     * 
     *     Bits ->
     * 0:  0  1  2  3  4  5  6  7
     * 1:  0  1  2  3  4  5  6  7
     * - New line (y+1) ---------
     * 2:  8  9 10 11 12 13 14 15
     * 3:  8  9 10 12 12 13 14 15
     * - New line (y+1) ---------
     * etc.
     */
    static const uint16_t morton_table[256] = {
        0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 
        0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 
        0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 
        0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155, 
        0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415, 
        0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 
        0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 
        0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555, 
        0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015, 
        0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055, 
        0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 
        0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 
        0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415, 
        0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455, 
        0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515, 
        0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555, 
        0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 
        0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055, 
        0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115, 
        0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155, 
        0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 
        0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 
        0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515, 
        0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555, 
        0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015, 
        0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055, 
        0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 
        0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155, 
        0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415, 
        0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455, 
        0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 
        0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
    };

    const size_t num = dst->remaining < TILE_WIDTH ?
        dst->remaining : TILE_WIDTH;

    // Interleave bytes, to get continuous bits
    const uint8_t line_high = src->tile->lines[src->y][0];
    const uint8_t line_low  = src->tile->lines[src->y][1];

    const uint16_t line =
        (morton_table[line_high] << 1) | morton_table[line_low];

    // Copy pixel for pixel
    for (size_t i = src->x; i < num; i++) {
        const int shift = (7 - i) * 2;
        dst->data[i] = palette->colors[(line & (0x3 << shift)) >> shift];
    }

    dst->data += num;
    dst->remaining -= num;
}

static void
draw_tiles(dest_t* restrict dst, const map_t* restrict map,
    const palette_t* restrict palette)
{
    map_t my_map = *map;

    // dst->remaining is updated by draw_tile
    while (dst->remaining > 0) {
        source_t src;
        size_t num;

        map_next(&my_map, &src);
        draw_tile(dst, &src, palette);
    }
}

static const tile_t*
get_tile_data(const memory_t *mem, uint16_t tile_id)
{
    assert(tile_id < 384);
    // This is guaranteed to be aligned
    return (tile_t*)&mem->gfx.tiles[tile_id * 16];
}

static void draw_line(context_t *ctx) {
    gfx_t* gfx = &ctx->gfx;
    uint8_t lcdc = ctx->mem.io.LCDC;
    uint8_t screen_y = ctx->mem.io.LY;

    map_t src;
    dest_t dest;
    palette_t palette;
    const palette_t* base_palette;
    
    if (!graphics_lock(ctx))
    {
        return;
    }
    
    // Background
    if (BIT_ISSET(lcdc, R_LCDC_BG_ENABLED))
    {
        base_palette = (gfx->debug_flags & LAYER_BACKGROUND) ?
            &gfx->debug_palettes[0] : &gfx->palette;

        palette_decode(&palette, base_palette, ctx->mem.io.BGP);

        map_init(
            &src, &ctx->mem,
            !BIT_ISSET(lcdc, R_LCDC_BG_TILE_MAP) ?
                &ctx->mem.gfx.map_low : &ctx->mem.gfx.map_high,
            ctx->mem.io.SCX, screen_y + ctx->mem.io.SCY
        );

        dest_init(
            &dest,
            (pixel_t*)gfx->background->pixels,
            0, screen_y, SCREEN_WIDTH
        );
        
        draw_tiles(&dest, &src, &palette);
    }
    
    // Window
    uint8_t window_y = ctx->mem.io.WY;
    // TODO: What happens on underflow?
    uint8_t window_x = ctx->mem.io.WX - 7;
    
    if (BIT_ISSET(lcdc, R_LCDC_WINDOW_ENABLED) && screen_y >= window_y &&
        window_x < SCREEN_WIDTH)
    {
        base_palette = (gfx->debug_flags & LAYER_WINDOW) ?
            &gfx->debug_palettes[1] : &gfx->palette;

        palette_decode(&palette, base_palette, ctx->mem.io.BGP);

        map_init(
            &src, &ctx->mem,
            !BIT_ISSET(lcdc, R_LCDC_WINDOW_TILE_MAP) ?
                &ctx->mem.gfx.map_low : &ctx->mem.gfx.map_high,
            window_x, screen_y + gfx->window_y
        );

        dest_init(
            &dest,
            (pixel_t*)gfx->background->pixels,
            window_x, screen_y, SCREEN_WIDTH
        );
        
        draw_tiles(&dest, &src, &palette);
        gfx->window_y += 1;
    }
    
    // Sprites
    if (BIT_ISSET(lcdc, R_LCDC_SPRITES_ENABLED))
    {
        size_t sprite_height = BIT_ISSET(lcdc, R_LCDC_SPRITES_LARGE) ? 16 : 8;
        palette_t spp_high, spp_low;

        base_palette = (gfx->debug_flags & LAYER_SPRITES) ?
            &gfx->debug_palettes[2] : &gfx->palette;

        palette_decode(&spp_high, base_palette, ctx->mem.io.SPP_HIGH);
        palette_decode(&spp_low, base_palette, ctx->mem.io.SPP_LOW);
        
        sprite_table_t sprites = { .length = 0 };
        
        for (int i = 0; i < MAX_SPRITES; i++)
        {
            sprite_t sprite;
            sprite_decode(&sprite, ctx->mem.gfx.oam[i]);

            if (!sprite.visible) {
                continue;
            }
            
            if (sprite.y <= screen_y && screen_y < sprite.y + sprite_height)
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

            uint16_t tile_id = (sprite->y < TILE_HEIGHT) ?
                (sprite->tile_id & 0xFE) : (sprite->tile_id | 0x01);

            source_init(
                &src,
                get_tile_data(&ctx->mem, tile_id),
                sprite->tile_x, sprite->tile_y
            );

            dest_init(
                &dst,
                sprite->in_background ?
                    (pixel_t*)gfx->sprites_bg->pixels :
                    (pixel_t*)gfx->sprites_fg->pixels,
                sprite->x, sprite->y, SCREEN_WIDTH
            );

            // TODO: Flipping

            draw_tile(
                &dst,
                &src,
                sprite->palette == PALETTE_HIGH ? &spp_high : &spp_low
            );
        }
    }

    graphics_unlock(ctx);
}

void graphics_sprite_table_add(sprite_table_t *table, const sprite_t* sprite)
{
    int i;

    assert(table != NULL);

    if (table->length == 0)
    {
        table->data[0] = *sprite;
        table->length = 1;
        
        return;
    }

    // Bail out if last (tenth) element is lower than element to be inserted
    if (table->length == SPRITES_PER_LINE &&
        table->data[SPRITES_PER_LINE - 1].x < sprite->x)
    {
        return;
    }

    i = (table->length >= SPRITES_PER_LINE - 1 ) ?
        SPRITES_PER_LINE - 1 : table->length;

    while (i > 0 && table->data[i - 1].x > sprite->x)
    {
        table->data[i] = table->data[i - 1];
        i--;
    }

    table->data[i] = *sprite;
    table->length += 1;
}

void graphics_draw_tile(const context_t* ctx, window_t* window,
    uint16_t tile_id, size_t x, size_t y)
{
    pixel_t* pixels = (pixel_t*)window->surface->pixels;

    for (size_t tile_y = 0; tile_y < TILE_HEIGHT; tile_y++) {
        source_t src;
        dest_t dst;

        dest_init(&dst, pixels, x, y + tile_y, window->surface->w);
        source_init(&src, get_tile_data(&ctx->mem, tile_id), 0, tile_y);
        draw_tile(&dst, &src, &ctx->gfx.debug_palettes[0]);
    }
}

void graphics_toggle_debug(context_t* ctx, graphics_layer_t layer)
{
    if (ctx->gfx.debug_flags & layer) {
        ctx->gfx.debug_flags &= ~layer;
    } else {
        ctx->gfx.debug_flags |= layer;
    }
}

bool graphics_get_debug(const context_t* ctx, graphics_layer_t layer)
{
    return ctx->gfx.debug_flags & layer;
}
