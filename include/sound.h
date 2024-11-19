#pragma once

#include <stdint.h>
#include <SDL2/SDL.h>

typedef struct memory memory_t;
typedef struct context context_t;

typedef struct {
	SDL_AudioDeviceID device;
} sound_t;

uint8_t sound_read(const memory_t *mem, uint16_t addr);
void sound_write(memory_t *mem, uint16_t addr, uint8_t value);
bool sound_init(sound_t *snd);
void sound_update(context_t *snd, unsigned int cycles);
