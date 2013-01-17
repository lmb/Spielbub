#include "graphics.h"
#include "ioregs.h"
#include "memory.h"
#include "cpu.h"
#include "hardware.h"
#include "logging.h"

#include <assert.h>
#include <SDL/SDL.h>

#define MAP_W (256)
#define MAP_H (256)

typedef enum {
    TILE_DATA_HIGH = 0x8800,
    TILE_DATA_LOW  = 0x8000
} tile_data_t;

typedef enum {
    TILE_MAP_HIGH = 0x9C00,
    TILE_MAP_LOW  = 0x9800
} tile_map_t;

/* BIG MESS, you have been warned */

void _draw_line();
void _put_pixel(uint32_t *screen_palette, uint32_t* buf, register uint8_t palette, register int x, register int data1, register int data2);

uint8_t _get_tile_id(const memory_t *mem, int x, int y, tile_map_t tile_map);
uint16_t* _get_tile_data(const memory_t *mem, uint8_t tile_id, tile_data_t tile_data);

// Sprite tables
void _add_sprite_to_table(sprite_table_t *table, sprite_t sprite);

/*
 * Sets a new LCD state and requests an
 * interrupt if appropriate.
 */
void _set_mode(context_t *ctx, gfx_state_t state)
{
    uint8_t* r_stat = mem_address(ctx->mem, R_STAT);
    ctx->gfx->state = state;

    *r_stat &= ~0x3;  // Clear bits 0-1
    *r_stat |= state; // Set new state

    if (state < 3 && BIT_ISSET(*r_stat, state + 3))
        cpu_irq(ctx, I_LCDC);
}

SDL_Surface* _init_surface(SDL_Color *colors)
{
    SDL_Surface *sfc = SDL_CreateRGBSurface(
        SDL_SWSURFACE | SDL_SRCCOLORKEY, SCREEN_W, SCREEN_H, 8, 0, 0, 0, 0);
    
    if (sfc == NULL) { return NULL; }
    
    SDL_SetColors(sfc, colors, 0, 4);
    SDL_SetColorKey(sfc, SDL_SRCCOLORKEY, SDL_MapRGB(sfc->format, 0xFF, 0x00, 0xFF));
    
    return sfc;
}

/*
 * Inits the SDL surface and palette colors.
 */
bool graphics_init(gfx_t* gfx)
{
    gfx->state = OAM;
    
    gfx->screen = SDL_SetVideoMode(
        SCREEN_W, SCREEN_H, 32 /*BPP*/,
        SDL_SWSURFACE
    );
    
    // Transparent, Light Grey, Dark Grey, Black
    SDL_Color colors[4] = {
        { .r = 0xFF, .g = 0x00, .b = 0xFF },
        { .r = 0xCC, .g = 0xCC, .b = 0xCC },
        { .r = 0x77, .g = 0x77, .b = 0x77 },
        { .r = 0x00, .g = 0x00, .b = 0x00 }
    };
    
    gfx->background = _init_surface(colors);
    gfx->sprites_bg = _init_surface(colors);
    gfx->sprites_fg = _init_surface(colors);
    
    gfx->screen_white = SDL_MapRGB(gfx->screen->format, 0xFF, 0xFF, 0xFF);

    return (gfx->screen != NULL);
}

bool graphics_lock(gfx_t *gfx)
{
    if (SDL_MUSTLOCK(gfx->screen) && SDL_LockSurface(gfx->screen) < 0)
    {
        log_dbg("Can not lock surface: %s", SDL_GetError());
        return false;
    }
    
    return true;
}

void graphics_unlock(gfx_t *gfx)
{
    if (SDL_MUSTLOCK(gfx->screen))
    {
        SDL_UnlockSurface(gfx->screen);
    }
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

    uint8_t* r_stat = mem_address(ctx->mem, R_STAT);
    if (!BIT_ISSET(mem_read(ctx->mem, R_LCDC), R_LCDC_ENABLED))
    {
        // LCD is disabled.
        mem_write(ctx->mem, R_LY, 144);
        ctx->gfx->state = VBLANK_WAIT;
        ctx->gfx->cycles = 0;

        return;
    }
    
    gfx_t *gfx = ctx->gfx;

    gfx->cycles += cycles;
    switch (gfx->state)
    {
        case OAM:
            gfx->state = OAM_WAIT;
            if (mem_read(ctx->mem, R_LY) == mem_read(ctx->mem, R_LYC))
            {
                // LY coincidence interrupt
                // The LCD just finished displaying a
                // line R_LY, which matches R_LYC.
                BIT_SET(*r_stat, R_STAT_LY_COINCIDENCE);
                if (BIT_ISSET(*r_stat, R_STAT_LYC_ENABLE))
                    cpu_irq(ctx, I_LCDC);
            }
            else
                BIT_RESET(*r_stat, R_STAT_LY_COINCIDENCE);

        case OAM_WAIT:
            if (gfx->cycles >= 80)
            {
                gfx->cycles -= 80;
                _set_mode(ctx, TRANSF);
            }
            break;

        case TRANSF:
            if (gfx->cycles >= 172)
            {
                gfx->cycles -= 172;
                _set_mode(ctx, HBLANK);
            }
            break;

        case HBLANK:
            _draw_line(ctx);
            gfx->state = HBLANK_WAIT;

        case HBLANK_WAIT:
            if (gfx->cycles >= 204)
            {
                gfx->cycles -= 204;
                if (mem_read(ctx->mem, R_LY) == 144)
                {
                    _set_mode(ctx, VBLANK);
                }
                else
                {
                    _set_mode(ctx, OAM);
                }
            }
            break;

        case VBLANK:
        {
            //SDL_Rect whole_screen = { .x = 0, .y = 0, .w = SCREEN_W, .h = SCREEN_H };
            
            SDL_FillRect(gfx->screen, NULL, gfx->screen_white);
            SDL_BlitSurface(gfx->sprites_bg, NULL, gfx->screen, NULL);
            SDL_BlitSurface(gfx->background, NULL, gfx->screen, NULL);
            SDL_BlitSurface(gfx->sprites_fg, NULL, gfx->screen, NULL);
            
            SDL_Flip(gfx->screen);
            
            SDL_FillRect(gfx->sprites_bg, NULL, gfx->sprites_bg->format->colorkey);
            SDL_FillRect(gfx->background, NULL, gfx->background->format->colorkey);
            SDL_FillRect(gfx->sprites_fg, NULL, gfx->sprites_fg->format->colorkey);
            
            cpu_irq(ctx, I_VBLANK);
            
            gfx->state = VBLANK_WAIT;
            gfx->window_y = 0;
        }

        case VBLANK_WAIT:
            if (gfx->cycles >= 456)
            {
                uint8_t* r_ly = mem_address(ctx->mem, R_LY);
                gfx->cycles -= 456;
                
                if ((*r_ly)++ == 153)
                {
                    *r_ly = 0;
                    _set_mode(ctx, OAM);
                    gfx->frame_rendered = true;
                }
            }
            break;
    }
}

/*
 * Dumps all tiles in tile memory. Not really
 * useful right now, since the screen is too small.
 */
//void grapics_debug_tiles(context_t *ctx)
//{
//    int i;
//    uint8_t* data = mem_address(ctx->mem, 0x8000);
//    uint8_t palette = mem_read(ctx->mem, R_BGP);
//
//    SDL_Rect r;
//    r.x = r.y = 0;
//    r.w = r.h = 256;
//    SDL_FillRect(ctx->gfx->screen, &r, SDL_MapRGB(ctx->gfx->screen->format, 255, 0, 255));
//
//    for (i = 0; i < 384; i++)
//    {
//        _put_tile(ctx->gfx, data + (i * 16), palette, (i % 16) * 8, (i / 16) * 8);
//    }
//
//    SDL_Flip(ctx->gfx->screen);
//}

//void _put_tile(const gfx_t *gfx, uint8_t* data, uint8_t palette, int x, int y)
//{
//    // Tiles are 8x8 pixels. Each line occupies 2 bytes.
//    // The two bytes are combined bit by bit, which gives
//    // the palette index, which is in turn mapped to a color.
//
//    int line;
//    SDL_Surface *sfc = gfx->screen;
//
//    // Write whole lines at once for speed.
//    for (line = 0; line < 8; line++)
//    {
//        int t = y + line;
//        uint8_t* dst = (uint8_t*)sfc->pixels + (y + line) * sfc->pitch + x;
//        int data1 = *(data + line * 2);
//        int data2 = *(data + line * 2 + 1);
//        // TODO: Can this be transformed into some kind of memcpy?
//        _put_pixel(gfx, dst, palette, 0, data1, data2);
//        _put_pixel(gfx, dst + 1, palette, 1, data1, data2);
//        _put_pixel(gfx, dst + 2, palette, 2, data1, data2);
//        _put_pixel(gfx, dst + 3, palette, 3, data1, data2);
//        _put_pixel(gfx, dst + 4, palette, 4, data1, data2);
//        _put_pixel(gfx, dst + 5, palette, 5, data1, data2);
//        _put_pixel(gfx, dst + 6, palette, 6, data1, data2);
//        _put_pixel(gfx, dst + 7, palette, 7, data1, data2);
//    }
//}

int _put_tile_line(SDL_Surface *screen, uint16_t *tile, uint8_t palette, int screen_x, int screen_y, int tile_x, int tile_y, bool transparent)
{
    assert(screen_x < SCREEN_W);
    assert(screen_y < SCREEN_H);
    
    int i;
    int bpp = screen->format->BytesPerPixel;
    uint8_t *pixels = (uint8_t*)(screen->pixels + (screen_y * screen->pitch) + (screen_x * bpp));
    
    tile += tile_y;
    
    int step;
    
    if (tile_x < 0) {
        tile_x *= -1;
        step = -1;
    }
    else
    {
        step = 1;
    }
    
    for (i = tile_x; i >= 0 && i < TILE_WIDTH; i += step)
    {
        if (screen_x + i >= SCREEN_W) { continue; }
        
        int index = (*tile) & (0x8080 >> i); // Get bit 16-i and 8-i
        index >>= 7 - i;                     // Right align
        index = index % 254;                 // Get rid of bits 7 to 1
        // index now holds a number between 0 and 3
        // Double the result to get the number of times we have to left shift below.
        index <<= 1;
        
        int color = ((palette & (0x3 << index)) >> index);
        
        if (color != 0 || !transparent) {
            *pixels = color;
        }
        pixels++;
    }
    
    return TILE_WIDTH - tile_x;
}

void _draw_bg_line(const context_t *ctx, int screen_x, int screen_y, int offset_x, int offset_y, int8_t palette, tile_map_t tile_map, tile_data_t tile_data)
{
    // Only a 160x144 "cutout" of the background is shown on the screen.
    // If the y-offset r_scy pushes this cutout off the screen it wraps
    // around.
    int bg_y = (screen_y + offset_y) % MAP_H;
    
    // Since bg_y doesn't necessarily start at a tile boundary, we have
    // to calculate at which y coordinate we are within a tile.
    int tile_y = bg_y % 8;
    
    while (screen_x < SCREEN_W)
    {
        // These calculations are analogous to bg_y et al above.
        int bg_x   = (screen_x + offset_x) % MAP_W;
        int tile_x = bg_x % 8;
        
        uint8_t tile_id = _get_tile_id(ctx->mem, bg_x, bg_y, tile_map);
        uint16_t *tile = _get_tile_data(ctx->mem, tile_id, tile_data);
        
        // Every line is 2 bytes
        screen_x += _put_tile_line(ctx->gfx->background, tile, palette, screen_x, screen_y, tile_x, tile_y, false);
    }
}

void _draw_sprite_line(context_t *ctx, sprite_t sprite, int screen_y)
{
    int sprite_y = screen_y - (sprite.y - SPRITE_HEIGHT);
    int screen_x = sprite.x - SPRITE_WIDTH;
    
    SDL_Surface *surface;
    
    if (screen_x >= 160) {
        // Off-screen sprites are simply ignored.
        return;
    }
    
    surface = BIT_ISSET(sprite.flags, SPRITE_F_TRANSLUCENT) ? ctx->gfx->sprites_bg : ctx->gfx->sprites_fg;
    
    uint16_t palette = BIT_ISSET(sprite.flags, SPRITE_F_HIGH_PALETTE) ? R_SPP_HIGH : R_SPP_LOW;
    palette = mem_read(ctx->mem, palette);
    
    int sprite_x = BIT_ISSET(sprite.flags, SPRITE_F_X_FLIP) ? -7 : 0;
    
    uint16_t *tile = _get_tile_data(ctx->mem, sprite.tile_id, TILE_DATA_LOW);
    
    _put_tile_line(surface, tile, palette,
        screen_x, screen_y, sprite_x, sprite_y, true);
}

/*
 * Draw a single line, as indicated in R_LY.
 * Occurs right before a HBLANK.
 */
void _draw_line(context_t *ctx) {
    // LCD control
    uint8_t r_lcdc = mem_read(ctx->mem, R_LCDC);
    uint8_t palette = mem_read(ctx->mem, R_BGP);
    
    uint8_t screen_y = mem_read(ctx->mem, R_LY);
    
    tile_data_t tile_data = BIT_ISSET(r_lcdc, R_LCDC_TILE_DATA) ? TILE_DATA_LOW : TILE_DATA_HIGH;
    
    if (!graphics_lock(ctx->gfx))
    {
        printf("_draw_line(): could not lock SDL surface\n");
        return;
    }
    
    // Background
    if (BIT_ISSET(r_lcdc, R_LCDC_BG_ENABLED))
    {
        uint8_t r_scx  = mem_read(ctx->mem, R_SCX);
        uint8_t r_scy  = mem_read(ctx->mem, R_SCY);
        
        tile_map_t bg_tile_map = !BIT_ISSET(r_lcdc, R_LCDC_BG_TILE_MAP) ?
            TILE_MAP_LOW : TILE_MAP_HIGH;
        
        _draw_bg_line(
            ctx,
            0, screen_y,
            r_scx, r_scy,
            palette, bg_tile_map, tile_data
        );
    }
    
    // Window
    uint8_t r_wy = mem_read(ctx->mem, R_WY);
    uint8_t r_wx = mem_read(ctx->mem, R_WX) - 7;
    
    if (BIT_ISSET(r_lcdc, R_LCDC_WINDOW_ENABLED) && screen_y >= r_wy && r_wx <= SCREEN_W - 1)
    {
        tile_map_t window_tile_map = !BIT_ISSET(r_lcdc, R_LCDC_WINDOW_TILE_MAP) ?
            TILE_MAP_LOW : TILE_MAP_HIGH;
        
        _draw_bg_line(
            ctx,
            r_wx, screen_y,
            0, ctx->gfx->window_y,
            palette, window_tile_map, tile_data
        );
            
        ctx->gfx->window_y += 1;
    }
    
    // Sprites
    if (BIT_ISSET(r_lcdc, R_LCDC_SPRITES_ENABLED))
    {
        int sprite_height = BIT_ISSET(r_lcdc, R_LCDC_SPRITES_LARGE) ? 16 : 8;
        
        int i;
        sprite_table_t sprites = { .length = 0 };
        sprite_t *oam = (sprite_t*)mem_address(ctx->mem, OAM_START);
        
        for (i = 0; i < OAM_ENTRIES; i++)
        {
            sprite_t sprite = *(oam + i);
            
            int real_sprite_y = sprite.y - 16;
            
            // OAM entries specify the y coordinate of the sprites'
            // lower border (?)
            if (screen_y >= real_sprite_y && screen_y < real_sprite_y + sprite_height)
            {
                _add_sprite_to_table(&sprites, sprite);
            }
        }
        
        assert(sprite_height == 8);
        
        // <sprites> holds the 10 sprites with highest priority,
        // sorted by ascending x coordinate. Since sprites
        // with lower x coords write over tiles with higher x coords
        // we draw in reverse order.
        for (i = sprites.length - 1; i >= 0; i--)
        {
            // TODO: Ignore LSB for 8x16 sprites
            _draw_sprite_line(ctx, sprites.data[i], screen_y);
        }
    }
        
    graphics_unlock(ctx->gfx);
    
    mem_write(ctx->mem, R_LY, screen_y + 1);
}

void _add_sprite_to_table(sprite_table_t *table, sprite_t sprite)
{
    int i;
    
    assert(table != NULL);
    
    if (table->length == 0)
    {
        table->data[0] = sprite;
        table->length = 1;
        
        return;
    }
    
    // Bail out if last (tenth) element is lower than element to be inserted
    if (table->length == SPRITES_PER_LINE &&
        table->data[SPRITES_PER_LINE - 1].x < sprite.x)
    {
        return;
    }
    
    i = (table->length > SPRITES_PER_LINE - 1 ) ?
        SPRITES_PER_LINE - 1 : table->length;
    while (i > 0 && table->data[i - 1].x > sprite.x)
    {
        table->data[i] = table->data[i - 1];
        i--;
    }
    
    table->data[i] = sprite;
    table->length += 1;
}

uint8_t _get_tile_id(const memory_t *mem, int x, int y, tile_map_t tile_map)
{
    int col = x / 8;
    int row = y / 8;
    
    // Tile IDs are stored in a 32x32 uint8_t array
    // Every y line add 32 bytes in offset
    int index = (row * 32) + col;
    assert (index < 32 * 32);
    
    return mem_read(mem, (int)tile_map + index);
}

uint16_t* _get_tile_data(const memory_t *mem, uint8_t tile_id, tile_data_t tile_data)
{
    int tile_addr;
    
    switch (tile_data) {
        case TILE_DATA_HIGH:
            // Tile IDs are signed
        {
            int tid_sign = (int8_t)tile_id;
            
            if (tid_sign < 0 || tid_sign > 128) {
                tid_sign = tid_sign;
            }
            
            // We add 128, since tile id #0 lies at 0x9000,
            // not 0x8800.
            tile_addr = (int8_t)tile_id + 128;
        }
            break;
            
        case TILE_DATA_LOW:
            // Tile IDs are unsigned
            tile_addr = tile_id;
            break;
    }
    
    return (uint16_t*)mem_address(mem, (int)tile_data + (tile_addr * 16));
}
