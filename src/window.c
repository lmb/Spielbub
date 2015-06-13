#include "window.h"

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB4444

bool window_init(window_t* window, const char name[], int w, int h)
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

    window->surface = SDL_CreateRGBSurface(
        0, SCREEN_WIDTH, SCREEN_HEIGHT, 16,
        0x0F00, 0x00F0, 0x000F, 0xF000
    );

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

window_t* window_create(const char name[], int w, int h)
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

void window_free(window_t* window)
{
    if (window != NULL) {
        window_destroy(window);
        free(window);
    }
}

void window_draw(window_t* window)
{
    SDL_UpdateTexture(window->texture, NULL,
        window->surface->pixels, window->surface->pitch);
    SDL_RenderClear(window->renderer);
    SDL_RenderCopy(window->renderer, window->texture, NULL, NULL);
    SDL_RenderPresent(window->renderer);
}
