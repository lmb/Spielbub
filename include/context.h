#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <stdbool.h>
#include <stdint.h>

#include "spielbub.h"

#include "cpu.h"
#include "memory.h"
#include "graphics.h"
#include "timers.h"

#include "buffers.h"

struct context {
    // CPU
    cpu_t cpu;
    
    // Memory
    memory_t mem;

    // Timer
    timers_t timers;

    // Graphics
    gfx_t gfx;

    // Point in time of the next run,
    // in ticks. Used to slow down
    // emulator if needed.
    uint32_t next_run;

    // Joypad
    // The state of both direction and "menu"
    // keys is saved here.
    uint8_t joypad_state;
    
    emulation_state_t state;
    bool running;

#if defined(DEBUG)
    circular_buffer* logs;
#endif

    update_func_t update_func;
    void* update_func_context;
};

bool context_init_minimal(context_t *ctx);

#endif//__CONTEXT_H__
