#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>

#include "context.h"
#include "timers.h"
#include "sound.h"

#define SAMPLE_RATE 44100
#define CYCLES_PER_SAMPLE (CLOCKSPEED * 1.0 / SAMPLE_RATE)
#define BUFFER_SIZE 4096
#define AMPLITUDE 127
#define FREQUENCY 440.0

int main() {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    
    const int waveform_cycles = 8; // one full waveform at max freq.

    context_t ctx;
    if (!context_init_minimal(&ctx)) {
        printf("unhappy\n");
        return 1;
    }
    
    if (!sound_init(&ctx.snd)) {
        printf("Sound failed\n");
        return 1;
    }
    
    // This is is what a ROM would do via CPU insns.
    ctx.mem.sound.square1.period_lsb = 0x40;
    ctx.mem.sound.square1.period_msb = 0x07;
    ctx.mem.sound.square1.duty = 0b10;

    // This is what the APU would do given a trigger.
    ctx.snd.square1.divider = 0x740;
    ctx.snd.square1.volume = 0b1111;
    
    printf("Playing something\n");
    
    double cycles = CYCLES_PER_SAMPLE;
    uint8_t buffer[BUFFER_SIZE];
    while (true) {
        if (SDL_GetQueuedAudioSize(ctx.snd.device) >= SAMPLE_RATE/4) {
            SDL_Delay(5);
            continue;
        }
        
        for (unsigned int i = 0; i < sizeof(buffer); i++) {
            sound_update(&ctx, cycles - fmod(cycles, 4));
            buffer[i] = ctx.snd.square1.value;

            cycles = CYCLES_PER_SAMPLE + fmod(cycles, 4);
        }

        if (SDL_QueueAudio(ctx.snd.device, buffer, sizeof(buffer)) != 0) {
            fprintf(stderr, "Error queueing audio: %s\n", SDL_GetError());
            break;
        }
    }
    
    SDL_CloseAudioDevice(ctx.snd.device);
    SDL_Quit();
    return 0;
}
