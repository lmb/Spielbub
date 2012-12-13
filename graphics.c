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
void _put_pixel(const gfx_t *gfx, uint32_t* buf, register uint8_t palette, register int x, register int data1, register int data2);

uint8_t _get_tile_id(const memory_t *mem, int x, int y, tile_map_t tile_map);
uint8_t* _get_tile_data(const memory_t *mem, uint8_t tile_id, tile_data_t tile_data);

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
    
    // White, Light Grey, Dark Grey, Black
    gfx->tile_palette[0] = SDL_MapRGB(gfx->screen->format, 0xFF, 0xFF, 0xFF);
    gfx->tile_palette[1] = SDL_MapRGB(gfx->screen->format, 0xCC, 0xCC, 0xCC);
    gfx->tile_palette[2] = SDL_MapRGB(gfx->screen->format, 0x77, 0x77, 0x77);
    gfx->tile_palette[3] = SDL_MapRGB(gfx->screen->format, 0x00, 0x00, 0x00);
    
    gfx->tile_palette[0] = SDL_MapRGB(gfx->screen->format, 0xFF, 0xFF, 0xFF);
    gfx->tile_palette[1] = SDL_MapRGB(gfx->screen->format, 0xCC, 0xCC, 0xCC);
    gfx->tile_palette[2] = SDL_MapRGB(gfx->screen->format, 0x77, 0x77, 0x77);
    gfx->tile_palette[3] = SDL_MapRGB(gfx->screen->format, 0x00, 0x00, 0x00);

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

    ctx->gfx->cycles += cycles;
    switch (ctx->gfx->state)
    {
        case OAM:
            ctx->gfx->state = OAM_WAIT;
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
            if (ctx->gfx->cycles >= 80)
            {
                ctx->gfx->cycles -= 80;
                _set_mode(ctx, TRANSF);
            }
            break;

        case TRANSF:
            if (ctx->gfx->cycles >= 172)
            {
                ctx->gfx->cycles -= 172;
                _set_mode(ctx, HBLANK);
            }
            break;

        case HBLANK:
            _draw_line(ctx);
            ctx->gfx->state = HBLANK_WAIT;

        case HBLANK_WAIT:
            if (ctx->gfx->cycles >= 204)
            {
                ctx->gfx->cycles -= 204;
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
            SDL_Flip(ctx->gfx->screen);
            
            cpu_irq(ctx, I_VBLANK);
            
            ctx->gfx->state = VBLANK_WAIT;
            ctx->gfx->window_y = 0;

        case VBLANK_WAIT:
            if (ctx->gfx->cycles >= 456)
            {
                uint8_t* r_ly = mem_address(ctx->mem, R_LY);
                ctx->gfx->cycles -= 456;
                
                if ((*r_ly)++ == 153)
                {
                    *r_ly = 0;
                    _set_mode(ctx, OAM);
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

/*
 * Puts the pixel at position x in the line data(1,2) into buf.
 */
void _put_pixel(const gfx_t *gfx, uint32_t* buf, uint8_t palette, int x, int data1, int data2)
{
    // Pixel 0 is in Bit 7 -> 0x80 >> tile_x
    // Bit from data2 is bit 1 in color -> 6 - tile_x
    // Bit from data1 is bit 0 in color -> 7 - tile_x
    int index =
            (((data2 & (0x80 >> x)) >> (7 - x)) << 1) |
            ((data1 & (0x80 >> x)) >> (7 - x));
    index *= 2;

    *buf = gfx->tile_palette[((palette & (0x3 << index)) >> index)];
}

//void __draw_line(context_t *ctx)
//{
//    SDL_Surface* screen = ctx->gfx->screen;
//
//    // Scroll background by x / y
//    uint8_t  r_scx  = mem_read(ctx->mem, R_SCX);
//    uint8_t  r_scy  = mem_read(ctx->mem, R_SCY);
//
//    // Window x & y position
//    uint8_t  r_wx   = mem_read(ctx->mem, R_WX) - 7;
//    uint8_t  r_wy   = mem_read(ctx->mem, R_WY);
//
//    // LCD control
//    uint8_t  r_lcdc = mem_read(ctx->mem, R_LCDC);
//    // Current line
//    uint8_t* r_ly   = mem_address(ctx->mem, R_LY);
//
//    // Background palette
//    register uint8_t  r_bgp  = mem_read(ctx->mem, R_BGP);
//
//    // Stores "pointers" to the correct tile data
//    uint8_t* bg_tile_map =
//        BIT_ISSET(r_lcdc, R_LCDC_BG_TILE_MAP) ?
//            //0x9C00 : 0x9800;
//            mem_address(ctx->mem, 0x9C00) : mem_address(ctx->mem, 0x9800);
//    uint8_t* window_tile_map =
//        BIT_ISSET(r_lcdc, R_LCDC_WINDOW_TILE_MAP) ?
//            mem_address(ctx->mem, 0x9C00) : mem_address(ctx->mem, 0x9800);
//
//    // If tiles are stored from 0x8800 onwards,
//    // the pointer in the tile map is signed.
//    bool signed_tile_id = !BIT_ISSET(r_lcdc, R_LCDC_TILE_DATA);
//
//    bool window_enabled = BIT_ISSET(r_lcdc, R_LCDC_WINDOW_ENABLED);
//
//    // Tile data basepointer
//    uint8_t* tile_data_base;
//    tile_data_base = signed_tile_id ?
//            mem_address(ctx->mem, 0x8800) : mem_address(ctx->mem, 0x8000);
//    
//    int screen_y = *r_ly;
//    int screen_x;
//
//    // TODO implement window support
//    assert(!window_enabled);
//    
//    if (!graphics_lock(ctx->gfx))
//    {
//        printf("graphics_lock() failed\n");
//        return;
//    }
//
//    // Draw background & window tiles
//    for (screen_x = 0; screen_x < 160; screen_x++)
//    {
//        int map_x, map_y;
//        int tile_row, tile_col, tile_id;
//        int tile_x, tile_y;
//        //uint16_t tile_map = bg_tile_map, tile_id_addr;
//        uint8_t* tile_data;
//
//        // Find correct entry in the tile map
//        map_y    = (screen_y + r_scy) % MAP_H;
//        map_x    = (screen_x + r_scx) % MAP_W;
//
//        tile_row = ((uint8_t)(map_y / 8)) * 32;
//        tile_col = (map_x / 8);
//
//        // Load tile id from tile map
////        tile_id_addr = tile_map + tile_row + tile_col;
////        if (signed_tile_id)
////            tile_id = ((int8_t)mem_read(ctx->mem, tile_id_addr))+ 128;
////        else
////            tile_id = mem_read(ctx->mem, tile_id_addr);
//        
//        tile_id = *(bg_tile_map + tile_row + tile_col);
//        if (signed_tile_id)
//            tile_id += 128;
//
//        // Load tile data
//        tile_y = map_y % 8;
//        tile_x = map_x % 8;
//
//        if (map_y % 8 == 0 && map_x % 8 == 0 && map_y > 15)
//        {
//            int i = 0;
//        }
//
//        tile_data = tile_data_base + tile_id * 16 + tile_y * 2;
//
//        _put_pixel(
//            ctx->gfx,
//            (uint8_t*)screen->pixels + screen_y * screen->pitch + screen_x,
//            r_bgp,
//            tile_x,
//            *tile_data,
//            *(tile_data + 1)
//        );
//    }
//
//    // TODO Is this correct?
//    (*r_ly)++;
//    
//    graphics_unlock(ctx->gfx);
//}

void _draw_bg(const context_t *ctx, int screen_x, int screen_y, int offset_x, int offset_y, int8_t palette, tile_map_t tile_map, tile_data_t tile_data)
{
    SDL_Surface *screen = ctx->gfx->screen;
    int bpp = screen->format->BytesPerPixel;
    
    // Only a 160x144 "cutout" of the background is shown on the screen.
    // If the y-offset r_scy pushes this cutout off the screen it wraps
    // around.
    int bg_y = (screen_y + offset_y) % MAP_H;
    
    // Since bg_y doesn't necessarily start at a tile boundary, we have
    // to calculate at which y coordinate we are within a tile.
    int tile_y = bg_y % 8;
    
    for (; screen_x < 160; screen_x++)
    {
        // These calculations are analogous to bg_y et al above.
        int bg_x   = (screen_x + offset_x) % MAP_W;
        int tile_x = bg_x % 8;
        
        uint8_t tile_id = _get_tile_id(ctx->mem, bg_x, bg_y, tile_map);
        uint8_t *tile = _get_tile_data(ctx->mem, tile_id, tile_data);
        
        // Every line is 2 bytes
        tile += tile_y * 2;
        
        _put_pixel(
           ctx->gfx,
           (uint32_t*)(screen->pixels + screen_y * screen->pitch + screen_x * bpp),
           palette,
           tile_x,
           *tile,
           *(tile + 1)
        );
    }
}

void _draw_sprite(context_t *ctx, sprite_t sprite, int screen_y)
{
    int y = screen_y - (sprite.b.y - SPRITE_HEIGHT);
    
    uint8_t palette = BIT_ISSET(sprite.b.flags, SPRITE_F_HIGH_PALETTE);
    
    
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
    
    if (BIT_ISSET(r_lcdc, R_LCDC_BG_AND_WIN_ENABLED))
    {
        // Background
        uint8_t r_scx  = mem_read(ctx->mem, R_SCX);
        uint8_t r_scy  = mem_read(ctx->mem, R_SCY);
        
        tile_map_t bg_tile_map = !BIT_ISSET(r_lcdc, R_LCDC_BG_TILE_MAP) ?
            TILE_MAP_LOW : TILE_MAP_HIGH;
        
        _draw_bg(
            ctx,
            0, screen_y,
            r_scx, r_scy,
            palette, bg_tile_map, tile_data
        );
        
        // Window
        uint8_t r_wy = mem_read(ctx->mem, R_WY);
        
        if (BIT_ISSET(r_lcdc, R_LCDC_WINDOW_ENABLED) && r_wy >= screen_y)
        {
            // TODO: Remove this as soon as this codepath is tested
            printf("window\n");
            
            tile_map_t window_tile_map = !BIT_ISSET(r_lcdc, R_LCDC_WINDOW_TILE_MAP) ?
            TILE_MAP_LOW : TILE_MAP_HIGH;
            
            uint8_t r_wx = mem_read(ctx->mem, R_WX) - 7;
            
            _draw_bg(
                ctx,
                r_wx, screen_y,
                0, ctx->gfx->window_y,
                palette, window_tile_map, tile_data
            );
            
            ctx->gfx->window_y += 1;
        }
    }
    
    if (BIT_ISSET(r_lcdc, R_LCDC_SPRITES_ENABLED))
    {
        int sprite_size = BIT_ISSET(r_lcdc, R_LCDC_SPRITES_LARGE) ? 32 : 16;
        
        int i;
        sprite_table_t sprites;
        sprite_t *oam = (sprite_t*)mem_address(ctx->mem, OAM_START);
        
        for (i = 0; i < OAM_ENTRIES; i++)
        {
            // byte 0: y coord
            // byte 1: x coord
            // byte 2: tile id
            // byte 3: flags
            sprite_t sprite = *(oam + OAM_ENTRY_SIZE * i);
            
            // OAM entries specify the y coordinate of the sprites'
            // lower border (?)
            if (sprite.b.y - 16 < screen_y && screen_y <= sprite.b.y)
            {
                _add_sprite_to_table(&sprites, sprite);
            }
        }
        
        // <sprites> holds the 10 sprites with highest priority,
        // sorted by ascending x coordinate. Since sprites
        // with lower x coords write over tiles with higher x coords
        // we draw in reverse order.
        for (i = SPRITES_PER_LINE - 1; i >= 0; i--)
        {
            
        }
    }
        
    graphics_unlock(ctx->gfx);
    
    mem_write(ctx->mem, R_LY, screen_y + 1);
}

#define TABLE_SPRITE_X(i) (table->data[i] & 0x00FF0000)

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
        table->data[SPRITES_PER_LINE - 1].b.x < sprite.b.x)
    {
        return;
    }
    
    i = (table->length > SPRITES_PER_LINE - 1 ) ?
        SPRITES_PER_LINE - 1 : table->length;
    while (i > 0 && table->data[i - 1].b.x > sprite.b.x)
    {
        table->data[i] = table->data[i - 1];
        i--;
    }
    
    table->data[i] = sprite;
    table->length += 1;
}

#undef TABLE_SPRITE_X

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

uint8_t* _get_tile_data(const memory_t *mem, uint8_t tile_id, tile_data_t tile_data)
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
    
    return mem_address(mem, (int)tile_data + (tile_addr * 16));
}
