#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <stdbool.h>
#include <stdint.h>

struct cpu_opaque_t;
typedef struct cpu_opaque_t cpu_t;

struct memory_opaque_t;
typedef struct memory_opaque_t memory_t;

struct gfx_opaque_t;
typedef struct gfx_opaque_t gfx_t;

struct timers_opaque_t;
typedef struct timers_opaque_t timers_t;

typedef enum {
    // Usually means that a breakpoint has been hit.
    STOPPED = 0,
    // Only execute one instruction at a time.
    SINGLE_STEPPING,
    // Emulation runs until program is quit or breakpoint etc. is hit.
    RUNNING
} emulation_state_t;

typedef struct {
    // CPU
    cpu_t *cpu;
    
    // Memory
    memory_t *mem;

    // Timer
    timers_t *timers;

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
    
    emulation_state_t state;
} context_t;

bool context_create(context_t *ctx);
bool context_init(context_t* ctx);
void context_destroy(context_t *ctx);
void context_run(context_t *context);

#endif//__CONTEXT_H__
