#ifndef __BIT_H__
#define __BIT_H__

#include <stdint.h>
#include <assert.h>

static inline bool
bit_is_set(uint8_t value, size_t bit)
{
	assert(bit < sizeof value * 8);
	return value & (1 << bit);
}

static inline uint8_t
bit_unset(uint8_t value, size_t bit)
{
	assert(bit < sizeof value * 8);
	return value & ~(1 << bit);
}

static inline uint8_t
bit_set(uint8_t value, size_t bit)
{
	assert(bit < sizeof value * 8);
	return value | (1 << bit);
}

#endif /* __BIT_H__ */
