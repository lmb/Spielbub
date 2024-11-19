#pragma once

#include <stdint.h>

typedef struct memory memory_t;

typedef struct {
	uint8_t temp;
} sound_t;

uint8_t sound_read(const memory_t *mem, uint16_t addr);
void sound_write(memory_t *mem, uint16_t addr, uint8_t value);
