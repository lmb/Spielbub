#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <stdbool.h>
#include <stdint.h>

#if defined(DEBUG)
#include "buffers.h"
#endif

struct cpu_opaque_t;
typedef struct cpu_opaque_t cpu_t;

struct memory_opaque_t;
typedef struct memory_opaque_t memory_t;

struct ipc_opaque_t;
typedef struct ipc_opaque_t ipc_t;

struct gfx_opaque_t;
typedef struct gfx_opaque_t gfx_t;

typedef struct {
    // CPU
    cpu_t *cpu;
    
    // Memory
    memory_t *mem;

    // Timer
    int timer_cycles;

    // Graphics
    gfx_t *gfx;

    // Point in time of the next run,
    // in ticks. Used to slow down
    // emulator if needed.
    uint32_t next_run;

    // Joypad
    // The state of both direction and "menu"
    // keys is saved here.
    uint8_t joypad_state;
    
    // IPC context
    ipc_t* ipc;

#if defined(DEBUG)
    circular_buffer* trace;
#endif
} context_t;

bool context_create(context_t *ctx);
bool context_init(context_t* ctx);
void context_destroy(context_t *ctx);
void run(context_t *context);

#endif//__CONTEXT_H__
