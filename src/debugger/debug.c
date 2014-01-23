#include <string.h>
#include <stdio.h>

#include "debugger/debug.h"

bool debug_init(debug_t* dbg)
{
    memset(dbg->commandline, 0, sizeof dbg->commandline);
    dbg->post_exec = &debug_post_exec_print_pc;
    // dbg->window = graphics_create_window("Tile DBG", 256, 256);

    if (dbg->window == NULL) {
        printf("Failed to create window\n");
        return false;
    }

    return true;
}

void debug_free(debug_t *dbg)
{
    // graphics_free_window(dbg->window);
}

void debug_print_func(const context_t* ctx, uint16_t addr)
{
    char buffer[256] = "";

    do {
        size_t len = context_decode_instruction(ctx, addr, buffer, sizeof buffer);
        printf("%04x: %s\n", addr, buffer);
        addr += len;
    } while (strncmp(buffer, "RET", 3) != 0);
}

void debug_print_addr(const context_t* ctx, uint16_t addr)
{
    char buffer[256] = "";
    context_decode_instruction(ctx, addr, buffer, sizeof buffer);
    printf("%04x: %s\n", addr, buffer);
}

void debug_post_exec_print_pc(context_t* ctx, debug_t* dbg)
{
    registers_t regs;

    context_get_registers(ctx, &regs);
    debug_print_addr(ctx, regs.PC);
}
