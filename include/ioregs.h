#ifndef __IOREGS_H__
#define __IOREGS_H__

#include <stdbool.h>

#include "memory.h"

typedef enum tile_map {
	TILE_MAP_LOW  = 0,
	TILE_MAP_HIGH = 1,
} tile_map_t;

static inline bool
tac_enabled(const memory_t *m)
{
	return m->io.TAC & (1<<2);
}

static inline unsigned int
tac_timer_type(const memory_t *m) {
	return m->io.TAC & 0x3;
}

static inline bool
lcdc_display_enabled(const memory_t *m)
{
	return m->io.LCDC & (1<<7);
}

static inline bool
lcdc_window_enabled(const memory_t *m)
{
	return m->io.LCDC & (1<<5);
}

static inline bool
lcdc_background_enabled(const memory_t *m)
{
	return m->io.LCDC & (1<<0);
}

static inline bool
lcdc_sprites_enabled(const memory_t *m)
{
	return m->io.LCDC & (1<<1);
}

static inline tile_map_t
lcdc_window_tile_map(const memory_t *m)
{
	return (m->io.LCDC & (1<<6)) ? TILE_MAP_HIGH : TILE_MAP_LOW;
}

static inline tile_map_t
lcdc_background_tile_map(const memory_t *m)
{
	return (m->io.LCDC & (1<<3)) ? TILE_MAP_HIGH : TILE_MAP_LOW;
}

static inline bool
lcdc_unsigned_tile_ids(const memory_t *m)
{
	return m->io.LCDC & (1<<4);
}

static inline size_t
lcdc_sprite_height(const memory_t *m)
{
	return (m->io.LCDC & (1<<2)) ? 16 : 8;
}

static inline bool
stat_lyc_irq_enabled(const memory_t *m)
{
	return m->io.STAT & (1<<6);
}

static inline void
stat_lyc_set(memory_t *m)
{
	m->io.STAT |= (1<<2);
}

static inline void
stat_lyc_clear(memory_t *m)
{
	m->io.STAT &= ~(1<<2);
}

#define BIT_SET(x, n) {x |= (1 << (n));}
#define BIT_RESET(x, n) {x &= ~(1 << (n));}
#define BIT_ISSET(x, n) (x & (1 << (n)))

#endif//__IOREGS_H__
