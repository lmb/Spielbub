#include "context.h"
#include "joypad.h"

#include "ioregs.h"

#define JOY_DOWN   SDL_SCANCODE_S
#define JOY_UP     SDL_SCANCODE_W
#define JOY_LEFT   SDL_SCANCODE_A
#define JOY_RIGHT  SDL_SCANCODE_D
#define JOY_START  SDL_SCANCODE_I
#define JOY_SELECT SDL_SCANCODE_O
#define JOY_A      SDL_SCANCODE_K
#define JOY_B      SDL_SCANCODE_L

static const SDL_Scancode keys[] = {
    JOY_A, JOY_B, JOY_SELECT, JOY_START,
    JOY_RIGHT, JOY_LEFT, JOY_UP, JOY_DOWN
};

static inline void _state_to_ioreg(context_t *ctx, uint8_t state, uint8_t *reg)
{
    if (state < (*reg & 0xF)) {
        cpu_irq(ctx, I_JOYPAD);
    }
    
    *reg = (*reg & 0xF0) | (state & 0xF);
}

void joypad_init(context_t *ctx)
{
    // Low nibble = control keys
    // High nibble = direction keys
    ctx->joypad_state = 0xFF;
}

void joypad_update(context_t *ctx)
{
    uint8_t *r_joypad = &ctx->mem.map[R_JOYPAD];
    uint8_t joy_state;
    
    // Active low
    if (!BIT_ISSET(*r_joypad, R_JOYPAD_CONTROL))
    {
        joy_state = (ctx->joypad_state) & 0xF;
        _state_to_ioreg(ctx, joy_state, r_joypad);
    }
    
    if (!BIT_ISSET(*r_joypad, R_JOYPAD_DIRECTION))
    {
        joy_state = (((ctx->joypad_state) & 0xF0) >> 4);
        _state_to_ioreg(ctx, joy_state, r_joypad);
    }
}

void joypad_update_state(context_t *ctx, const SDL_KeyboardEvent *evt)
{
    // This function will be called roughly 60 times a second with all
    // pending keyboard events, one by one.
    
    // Remember that the GB joypad is active low.
    
    int i;
    
    for (i = 0; i < 8; i++)
    {
        const SDL_Scancode key = keys[i];

        if (evt->keysym.scancode == key)
        {
            if (evt->state == SDL_PRESSED)
            {
                // Active low
                BIT_RESET(ctx->joypad_state, i);
            }
            else
            {
                BIT_SET(ctx->joypad_state, i);
            }
            
            break;
        }
    }
}

/*
 * Updates the joypad state, every time a key is pressed.
 */
//void joypad_update_state(context_t *ctx, SDL_KeyboardEvent *evt)
//{
//    int i;
//
//    // Loop through keys, to see if they match.
//    for (i = 0; i < 8; i++)
//    {
//        if (evt->keysym.sym == keys[i])
//        {
//            int mask     = (1 << i);
//            int r_joypad = mem_read(ctx->mem, R_JOYPAD);
//
//            if (evt->state == SDL_PRESSED)
//            {
//                // If a key is pressed, it's corresponding
//                // bit in joypad_state is set to 0...
//
//                if (BIT_ISSET(ctx->joypad_state, i))
//                {
//                    // Key pin changed from high -> low
//                    if (
//                        ((i > 3) && BIT_ISSET(ctx->joypad_state, R_JOYPAD_DIRECTION)) ||
//                        ((i < 4) && BIT_ISSET(ctx->joypad_state, R_JOYPAD_BUTTONS))
//                    ) {
//                        // Only request an interrupt when the corresponding
//                        // bit in R_JOYPAD is set.
//                        cpu_irq(ctx, I_JOYPAD);
//                    }
//
//                    BIT_RESET(ctx->joypad_state, i);
//                }
//            }
//            else
//                // ...and vice versa.
//                BIT_SET(ctx->joypad_state, i);
//
//            return;
//        }
//    }
//}
