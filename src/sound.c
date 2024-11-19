#include <string.h>
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "memory.h"
#include "ioregs.h"
#include "sound.h"

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 4096
#define AMPLITUDE 127

bool sound_init(sound_t *snd) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = BUFFER_SIZE;
    want.callback = NULL;
    
    snd->device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (snd->device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    SDL_PauseAudioDevice(snd->device, 0);
    return true;
}

void sound_update(context_t *ctx, unsigned int cycles) {
    (void)ctx; (void)cycles;
}

uint8_t sound_read(const memory_t *mem, uint16_t addr)
{
    // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Register_Reading
    const uint8_t mask[] = {
    //  NRx0 NRx1 NRx2 NRx3 NRx4
    // ---------------------------
        0x80,  0x3F, 0x00,  0xFF,  0xBF,  // NR1x
        0xFF,  0x3F, 0x00,  0xFF,  0xBF,  // NR2x
        0x7F,  0xFF, 0x9F,  0xFF,  0xBF,  // NR3x
        0xFF,  0xFF, 0x00,  0x00,  0xBF,  // NR4x
        0x00,  0x00, 0x70,                // NR5x
    };
    
    switch (addr) {
    case range_of(memory_sound_t, regs): {
        int index = addr - offsetof(memory_sound_t, NR10);
        return mem->map[addr] | mask[index];
    }
    case range_of(memory_sound_t, unused):
        return 0xff;
    default:
        // TODO: Not sure this is correct.
        return mem->map[addr];
    }
}

void sound_write(memory_t *mem, uint16_t addr, uint8_t value)
{
    switch (addr) {
    case offsetof(memory_sound_t, NR52):
        if (BIT_ISSET(value, 7)) {
            // Reset frame sequencer
            // Reset square duty units
            // Reset wave channel buffer
            mem->sound.power = 1;
        } else {
            memset(mem->sound.regs, 0, sizeof(mem->sound.regs));
        }
        break;
        
    case range_of(memory_sound_t, unused):
        return;
        
    default:
        if (!mem->sound.power) {
            // Any writes are ignored when powered off.
            // TODO: Allow writing length counters.
            return;
        }
        
        // TODO: Wildly inaccurate.
        mem->map[addr] = value;
    }
}
