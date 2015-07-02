#ifndef __GRAPHICS_TILES_H__
#define __GRAPHICS_TILES_H__

#include "context.h"

typedef struct dest {
    uint8_t* data;
    size_t remaining;
} dest_t;

typedef struct source {
    const memory_tile_t* tile;
    size_t x, y;
} source_t;

typedef struct map {
    const memory_tile_map_t* tile_map;
    const memory_tile_data_t* tile_data;
    bool signed_ids;

    size_t row, col;
    size_t tile_x, tile_y;
} map_t;

void map_init(map_t* map, const memory_t* mem, tile_map_t tile_map,
    size_t x, size_t y);

void map_next(map_t* map, source_t* src);

void draw_tile(dest_t* restrict dst, const source_t* restrict src,
    palette_t palette);
void draw_tiles(dest_t* restrict dst, const map_t* restrict map,
    palette_t palette);

void dest_init(dest_t* dst, SDL_Surface* surface, size_t x, size_t y, size_t num);
void source_init(source_t *src, const memory_tile_t* tile, size_t x, size_t y);

#endif//__GRAPHICS_TILES_H__
