#include "context.h"
#include "joypad.h"

#define CONTROL_SELECTED   (1<<5)
#define DIRECTION_SELECTED (1<<4)

#define NUM(x) (sizeof (x) / sizeof (x)[0])

static const struct {
    SDL_Scancode sdl_key;
    joypad_key_t key;
} keymap[] = {
    { SDL_SCANCODE_K, KEY_A },
    { SDL_SCANCODE_L, KEY_B },
    { SDL_SCANCODE_O, KEY_SELECT },
    { SDL_SCANCODE_I, KEY_START },
    { SDL_SCANCODE_D, KEY_RIGHT },
    { SDL_SCANCODE_A, KEY_LEFT },
    { SDL_SCANCODE_W, KEY_UP },
    { SDL_SCANCODE_S, KEY_DOWN }
};

static joypad_key_t map_sdl_key(SDL_Scancode sdl_key)
{
    for (size_t i = 0; i < NUM(keymap); i++) {
        if (keymap[i].sdl_key == sdl_key) {
            return keymap[i].key;
        }
    }

    return KEY_INVALID;
}

static void
state_to_ioreg(context_t *ctx, uint8_t state)
{
    ctx->mem.io.JOYPAD &= 0xF0;
    ctx->mem.io.JOYPAD |= (state & 0xF);
}

void joypad_init(context_t *ctx)
{
    ctx->joypad_state = 0xFF;
}

void joypad_press(context_t* ctx, joypad_key_t key) {
    if (key != KEY_INVALID) {
        ctx->joypad_state &= ~key;
        cpu_irq(ctx, I_JOYPAD);
    }
}

void joypad_release(context_t* ctx, joypad_key_t key) {
    ctx->joypad_state |= key;
}

void joypad_update(context_t *ctx)
{
    // Active low
    if (!(ctx->mem.io.JOYPAD & CONTROL_SELECTED))
    {
        state_to_ioreg(ctx, ctx->joypad_state);
    }
    
    if (!(ctx->mem.io.JOYPAD & DIRECTION_SELECTED))
    {
        state_to_ioreg(ctx, ctx->joypad_state >> 4);
    }
}

void joypad_update_state(context_t *ctx, const SDL_KeyboardEvent *evt)
{
    // This function will be called roughly 60 times a second with all
    // pending keyboard events, one by one.

    joypad_key_t key = map_sdl_key(evt->keysym.scancode);

    if (evt->state == SDL_PRESSED) {
        joypad_press(ctx, key);
    } else {
        joypad_release(ctx, key);
    }
}
