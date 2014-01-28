#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "spielbub.h"

#define DBG_TILES_PER_ROW (32)

typedef struct debug debug_t;

typedef void (*command_t)(const char* args, context_t* ctx, debug_t* dbg);

struct debug {
    window_t* window;
    bool show_tiles;

    char commandline[256];
    command_t previous_command;
};

bool debug_init(debug_t* dbg);
void debug_free(debug_t *dbg);
void debug_draw_tiles(const context_t* ctx, debug_t* dbg);
void debug_print_func(const context_t* ctx, uint16_t addr);
void debug_print_addr(const context_t* ctx, uint16_t addr);
void debug_print_pc(context_t* ctx);

#endif//__DEBUG_H__
