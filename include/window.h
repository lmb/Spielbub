#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <SDL2/SDL.h>

#include "spielbub.h"

struct window {
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;
    SDL_Surface*  surface;
};

bool window_init(window_t* window, const char name[], int w, int h);
void window_destroy(window_t* window);

#endif//__WINDOW_H__
