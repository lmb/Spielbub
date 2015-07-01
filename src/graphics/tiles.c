#include <assert.h>

#include "ioregs.h"
#include "graphics/tiles.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void
map_init(map_t* map, const memory_t* mem, const memory_tile_map_t* tile_map,
    size_t x, size_t y)
{
    x %= MAP_WIDTH;
    y %= MAP_HEIGHT;

    map->signed_ids = !BIT_ISSET(mem->io.LCDC, R_LCDC_TILE_DATA);

    map->tile_map = tile_map;
    map->tile_data = &mem->gfx.tiles;

    map->row    = y / TILE_HEIGHT;
    map->col    = x / TILE_WIDTH;
    map->tile_y = y % TILE_HEIGHT;
    map->tile_x = x % TILE_WIDTH;
}

void
map_next(map_t* map, source_t* src)
{
    uint8_t tile_id = map->tile_map->data[map->row][map->col];
    size_t index;

    if (map->signed_ids) {
        index = 256 + (int8_t)tile_id;
    } else {
        index = tile_id;
    }

    assert(index < MAX_TILES);

    src->tile = &map->tile_data->data[index];
    src->x = map->tile_x;
    src->y = map->tile_y;

    // Only the first tile can have a non-zero x offset
    map->tile_x = 0;
    map->col = (map->col + 1) % MAP_COLUMNS;
}

void
draw_tile(dest_t* restrict dst, const source_t* restrict src, palette_t palette)
{
    /* Tiles are 8x8 pixels, with every pixel taking up two bits (for four
     * colors). The pixel bits are not continuous in memory, but split over
     * two bytes.
     * 
     *     Bits ->
     * 0:  0  1  2  3  4  5  6  7
     * 1:  0  1  2  3  4  5  6  7
     * - New line (y+1) ---------
     * 2:  8  9 10 11 12 13 14 15
     * 3:  8  9 10 12 12 13 14 15
     * - New line (y+1) ---------
     * etc.
     */
    static const uint16_t morton_table[256] = {
        0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 
        0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 
        0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 
        0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155, 
        0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415, 
        0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 
        0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 
        0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555, 
        0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015, 
        0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055, 
        0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 
        0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 
        0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415, 
        0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455, 
        0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515, 
        0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555, 
        0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 
        0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055, 
        0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115, 
        0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155, 
        0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 
        0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 
        0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515, 
        0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555, 
        0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015, 
        0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055, 
        0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 
        0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155, 
        0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415, 
        0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455, 
        0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 
        0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
    };

    const size_t num = dst->remaining < TILE_WIDTH ?
        dst->remaining : TILE_WIDTH;

    // Interleave bytes, to get continuous bits
    const uint8_t line_high = src->tile->lines[src->y][0];
    const uint8_t line_low  = src->tile->lines[src->y][1];

    const uint16_t line =
        (morton_table[line_high] << 1) | morton_table[line_low];

    // Copy pixel for pixel
    for (size_t i = src->x; i < num; i++, dst->data++, dst->remaining--) {
        const int shift = (7 - i) * 2;
        *dst->data = palette.colors[(line & (0x3 << shift)) >> shift];
    }
}

void
draw_tiles(dest_t* restrict dst, const map_t* restrict map, palette_t palette)
{
    map_t my_map = *map;

    // dst->remaining is updated by draw_tile
    while (dst->remaining > 0) {
        source_t src;
        size_t num;

        map_next(&my_map, &src);
        draw_tile(dst, &src, palette);
    }
}

void
dest_init(dest_t* dst, SDL_Surface* surface, size_t x, size_t y, size_t num)
{
    size_t max_pixels = ((size_t)surface->w) - x;
    assert(y < (size_t)surface->h);

    dst->data = ((uint8_t*)surface->pixels) + (y * surface->w) + x;
    dst->remaining = MIN(num, max_pixels);
}

void
source_init(source_t *src, const memory_tile_t* tile, size_t x, size_t y)
{
    src->tile = tile;
    src->x = x % TILE_WIDTH;
    src->y = y % TILE_HEIGHT;
}
