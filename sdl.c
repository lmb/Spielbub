#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 4096
#define AMPLITUDE 127
#define FREQUENCY 440.0
#define MAX_QUEUED_SIZE (BUFFER_SIZE * 4) // Limit queue size to prevent excessive buffering

int main() {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
     
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = BUFFER_SIZE;
    want.callback = NULL;
    
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    Uint8* buffer = (Uint8*)malloc(BUFFER_SIZE * sizeof(Uint8));
    double phase = 0.0;
    bool running = true;
    
    SDL_PauseAudioDevice(device, 0);
    printf("Playing continuous tone...\n");
    
    while (running) {
        // Only queue more audio if we're below our maximum queue size
        if (SDL_GetQueuedAudioSize(device) < MAX_QUEUED_SIZE) {
            for (int i = 0; i < BUFFER_SIZE; i++) {
                buffer[i] = (Uint8)(AMPLITUDE * sin(phase) + 128);
                phase += 2.0 * M_PI * FREQUENCY / SAMPLE_RATE;
                if (phase > 2.0 * M_PI) {
                    phase -= 2.0 * M_PI;
                }
            }
            
            if (SDL_QueueAudio(device, buffer, BUFFER_SIZE * sizeof(Uint8)) != 0) {
                fprintf(stderr, "Error queueing audio: %s\n", SDL_GetError());
                break;
            }
        }
        
        SDL_Delay(5); // Short delay to prevent CPU overuse
    }
    
    free(buffer);
    SDL_CloseAudioDevice(device);
    SDL_Quit();
    return 0;
}
