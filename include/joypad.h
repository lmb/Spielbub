#ifndef __JOYPAD_H__
#define __JOYPAD_H__

#include "spielbub.h"

#include <SDL2/SDL.h>

void joypad_init(context_t *ctx);
void joypad_update(context_t *ctx);
void joypad_update_state(context_t *ctx, const SDL_KeyboardEvent *evt);

#endif//__JOYPAD_H__
