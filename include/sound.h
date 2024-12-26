#pragma once

#include <stdint.h>
#include <SDL2/SDL.h>

#include "bitfield.h"

typedef struct memory memory_t;
typedef struct context context_t;

typedef struct {
	uint8_t BITFIELD(:1, sweep:3, negate:1, shift:3);
    uint8_t BITFIELD(duty:2, length_load:6);
    uint8_t BITFIELD(volume:4, envelope_mode:1, period:3);
    uint8_t period_lsb;
    uint8_t BITFIELD(trigger:1, length_enable:1, :3, period_msb:3);
} sound_square_params_t;

typedef struct {
	uint16_t divider;
	uint8_t wave_step;
	uint8_t value;
	uint8_t volume;
} sound_square_state_t;

typedef struct {
	SDL_AudioDeviceID device;
	sound_square_state_t square1;
} sound_t;

uint8_t sound_read(const context_t *ctx, uint16_t addr);
void sound_write(context_t *ctx, uint16_t addr, uint8_t value);
bool sound_init(sound_t *snd);
void sound_update(context_t *ctx, unsigned int cycles);
void sound_update_square(sound_square_state_t *ch, const sound_square_params_t *params);
