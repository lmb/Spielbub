#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <SDL2/SDL.h>

#include "spielbub.h"
#include "hardware.h"

typedef enum {
    HBLANK = 0x00, VBLANK, OAM, TRANSF, HBLANK_WAIT, VBLANK_WAIT, OAM_WAIT
} gfx_state_t;

typedef struct gfx {
    // Number of cycles in the current state.
    int          cycles;
    
    // Y position within the GB background window
    // Reset on every VBLANK
    int          window_y;
    
    // Current state.
    gfx_state_t  state;
    
    bool frame_rendered;
    
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;

    uint16_t palette[4];

    SDL_Surface* screen;
    SDL_Surface* sprites_bg;
    SDL_Surface* sprites_fg;
    SDL_Surface* background;

    uint32_t screen_white;
} gfx_t;

// This is completely unportable.
typedef union {
    uint32_t raw;
    
    struct {
        // X & Y Pos of the bottom right corner, for an 8x16 sprite
        uint8_t y;
        uint8_t x;
        uint8_t tile_id;
        uint8_t flags;
    };
    
    // Clang doesn't support unnamed structs
    // in designazed initializers.
    struct {
        uint8_t y, x, tile_id, flags;
    } b;
} sprite_t;

#define SPRITE_F_HIGH_PALETTE (4)
#define SPRITE_F_X_FLIP (5)
#define SPRITE_F_Y_FLIP (6)
#define SPRITE_F_TRANSLUCENT (7)

typedef struct {
    int length;
    sprite_t data[SPRITES_PER_LINE];
} sprite_table_t;

bool graphics_init(gfx_t *gfx);
bool graphics_lock(context_t *ctx);
void graphics_unlock(context_t *ctx);
void graphics_update(context_t *ctx, int cycles);
// TODO: Const this?
//void grapics_debug_tiles(context_t *context);

#endif//__GRAPHICS_H__
