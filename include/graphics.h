#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <SDL2/SDL.h>

#include "spielbub.h"

#define SPRITES_PER_LINE (10)
#define MAP_WIDTH (256)
#define MAP_HEIGHT (256)

typedef enum {
    HBLANK = 0x00, VBLANK, OAM, TRANSF, HBLANK_WAIT, VBLANK_WAIT, OAM_WAIT
} gfx_state_t;

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB4444
typedef uint16_t pixel_t;

typedef struct palette {
    pixel_t colors[4];
} palette_t;

struct window {
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;
    SDL_Surface*  surface;
};

typedef struct gfx {
    // Number of cycles in the current state.
    int          cycles;
    
    // Y position within the GB background window
    // Reset on every VBLANK
    int          window_y;
    
    // Current state.
    gfx_state_t  state;
    
    window_t window;

    palette_t palette;
    palette_t debug_palettes[3];
    int debug_flags;

    SDL_Surface* sprites_bg;
    SDL_Surface* sprites_fg;
    SDL_Surface* background;

    uint32_t screen_white;
} gfx_t;

typedef struct sprite {
    size_t x, y;

    bool visible;
    bool in_background;
    bool flip_x, flip_y;
    enum { PALETTE_HIGH, PALETTE_LOW } palette;

    uint16_t tile_id;
    size_t tile_x, tile_y;
} sprite_t;

typedef struct {
    int length;
    sprite_t data[SPRITES_PER_LINE];
} sprite_table_t;

bool graphics_init(gfx_t *gfx);
void graphics_destroy(gfx_t *gfx);
bool graphics_lock(context_t *ctx);
void graphics_unlock(context_t *ctx);
void graphics_update(context_t *ctx, int cycles);
void graphics_sprite_table_add(sprite_table_t *table, const sprite_t* sprite);

#endif//__GRAPHICS_H__
