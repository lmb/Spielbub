#ifndef __SPIELBUB_H__
#define __SPIELBUB_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct context context_t;
typedef struct window window_t;
typedef void (*update_func_t)(context_t*, void*);

typedef enum emulation_state {
    // Initial state
    STOPPED = 0,
    // Only execute one instruction at a time.
    SINGLE_STEPPED,
    // Executions breakpoint was hit
    BREAKPOINT,
    // Emulation runs until program is quit or breakpoint etc. is hit.
    RUNNING
} execution_state_t;

typedef struct registers {
    uint16_t AF, BC, DE, HL, SP, PC;
} registers_t;

context_t* context_create(update_func_t func, void* context);
bool context_load_rom(context_t *ctx, const char* filename);
void context_destroy(context_t *ctx);
void context_quit(context_t* ctx);
bool context_run(context_t* ctx);

size_t context_decode_instruction(const context_t* ctx, uint16_t addr,
    char dst[], size_t len);

void context_resume_exec(context_t* ctx);
void context_pause_exec(context_t* ctx);
void context_single_step(context_t* ctx);
execution_state_t context_get_exec(context_t* ctx);

bool context_add_breakpoint(context_t* ctx, uint16_t addr);

void context_get_registers(const context_t* ctx, registers_t* regs);
uint16_t context_get_memory(const context_t* ctx, uint8_t buffer[],
    uint16_t addr, uint16_t len);

void context_reset_traceback(const context_t* ctx);
bool context_get_traceback(const context_t* ctx, uint16_t* value);

window_t* graphics_create_window(const char name[], int w, int h);
void graphics_free_window(window_t* window);
// void graphics_render_tile(const context_t* ctx, uint16_t tile_id, pixel_t* pixels);


#endif//__SPIELBUB_H__
