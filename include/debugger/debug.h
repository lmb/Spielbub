#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "spielbub.h"

typedef struct debug debug_t;

typedef void (*command_t)(const char* args, context_t* ctx, debug_t* dbg);

struct debug {
    window_t* window;

    char commandline[256];
    command_t previous_command;

    void (*post_exec)(context_t*, debug_t* dbg);
};

bool debug_init(debug_t* dbg);
void debug_free(debug_t *dbg);
void debug_print_func(const context_t* ctx, uint16_t addr);
void debug_print_addr(const context_t* ctx, uint16_t addr);
void debug_post_exec_print_pc(context_t* ctx, debug_t* dbg);

#endif//__DEBUG_H__
