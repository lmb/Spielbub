#ifndef __SPIELBUB_H__
#define __SPIELBUB_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define SCREEN_HEIGHT   (144)
#define SCREEN_WIDTH   (160)
#define SPRITE_HEIGHT (16)
#define SPRITE_WIDTH (8)
#define TILE_HEIGHT (8)
#define TILE_WIDTH (8)
#define MAP_COLUMNS (32)
#define MAP_ROWS (32)
#define MAX_TILES (384)
#define MAX_SPRITES (40)

typedef struct context context_t;
typedef struct window window_t;
typedef void (*update_func_t)(context_t*, void*);

typedef enum emulation_state {
    // Initial state
    STOPPED = 0,
    // Only execute one instruction at a time.
    SINGLE_STEPPED,
    // Draw one frame at a time
    FRAME_STEPPED,
    // Executions breakpoint was hit
    BREAKPOINT,
    // Emulation runs until program is quit or breakpoint etc. is hit.
    RUNNING
} execution_state_t;

typedef enum graphics_layer {
    LAYER_BACKGROUND = 1,
    LAYER_WINDOW = 2,
    LAYER_SPRITES = 3
} graphics_layer_t;

typedef enum joypad_key {
    KEY_INVALID = 0,
    KEY_A = 1,
    KEY_B = 2,
    KEY_SELECT = 4,
    KEY_START = 8,
    KEY_RIGHT = 16,
    KEY_LEFT = 32,
    KEY_UP = 64,
    KEY_DOWN = 128
} joypad_key_t;

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
void context_frame_step(context_t* ctx);
execution_state_t context_get_exec(context_t* ctx);

bool context_add_breakpoint(context_t* ctx, uint16_t addr);

void context_get_registers(const context_t* ctx, registers_t* regs);
uint16_t context_get_memory(const context_t* ctx, uint8_t buffer[],
    uint16_t addr, uint16_t len);

void context_reset_traceback(const context_t* ctx);
bool context_get_traceback(const context_t* ctx, uint16_t* value);

void joypad_press(context_t* ctx, joypad_key_t key);
void joypad_release(context_t* ctx, joypad_key_t key);

window_t* window_create(const char name[], int w, int h);
void window_free(window_t* window);
void window_draw(window_t* window);

void graphics_toggle_debug(context_t* ctx, graphics_layer_t layer);
bool graphics_get_debug(const context_t* ctx, graphics_layer_t layer);
void graphics_draw_tile(const context_t* ctx, window_t* window,
    uint16_t tile_id, size_t x, size_t y);


#endif//__SPIELBUB_H__
