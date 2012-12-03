#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <SDL/SDL_video.h>

#include "context.h"

typedef enum {
    HBLANK = 0x00, VBLANK, OAM, TRANSF, HBLANK_WAIT, VBLANK_WAIT, OAM_WAIT
} gfx_state_t;

struct gfx_opaque_t
{
    // Number of cycles in the current state.
    int          cycles;
    
    // Y position within the GB background window
    // Reset on every VBLANK
    int          window_y;
    
    // Current state.
    gfx_state_t  state;
    
    SDL_Surface* screen;
    
    // Precomputed colors.
    uint8_t      colors[4];
};

typedef struct {
    int length;
    uint32_t data[10];
} sprite_table_t;

bool graphics_init(gfx_t *gfx);
bool graphics_lock(gfx_t *gfx);
void graphics_unlock(gfx_t *gfx);
void graphics_update(context_t *ctx, int cycles);
// TODO: Const this?
void grapics_debug_tiles(context_t *context);

#endif//__GRAPHICS_H__
