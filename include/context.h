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

#define CYCLES_PER_FRAME (70886)
#define TICKS_PER_FRAME (17)

typedef enum stopflags {
    STOP_STEP = 1,
    STOP_FRAME = 2,
} stopflags_t;

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
    uint8_t joypad_state;
    
    execution_state_t state;
    bool running;

#if defined(DEBUG)
    circular_buffer* logs;
    circular_buffer* traceback;
    prob_list_t breakpoints;
    int stopflags;
#endif

    update_func_t update_func;
    void* update_func_context;
};

bool context_init_minimal(context_t *ctx);

#endif//__CONTEXT_H__
