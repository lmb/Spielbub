#include <string.h>
#include <stdio.h>

#include "debugger/debug.h"

bool debug_init(debug_t* dbg)
{
    memset(dbg->commandline, 0, sizeof dbg->commandline);
    return true;
}

void debug_free(debug_t *dbg)
{
    if (dbg->window != NULL) {
        graphics_free_window(dbg->window);
    }
}

void debug_draw_tiles(const context_t* ctx, debug_t* dbg)
{
    for (size_t i = 0; i < MAX_TILES; i++) {
        size_t x = (i % DBG_TILES_PER_ROW) * (TILE_WIDTH + 1);
        size_t y = (i / DBG_TILES_PER_ROW) * (TILE_HEIGHT + 1);

        graphics_draw_tile(ctx, dbg->window, i, x, y);
    }

    graphics_draw_window(dbg->window);
}

void debug_print_func(const context_t* ctx, uint16_t addr)
{
    char buffer[256] = "";
    size_t count = 0;

    do {
        size_t len = context_decode_instruction(ctx, addr, buffer, sizeof buffer);
        printf("%04Xh: %s\n", addr, buffer);
        addr += len;
    } while (strncmp(buffer, "RET", 3) != 0 || count++ < 20);
}

void debug_print_addr(const context_t* ctx, uint16_t addr)
{
    char buffer[256] = "";
    context_decode_instruction(ctx, addr, buffer, sizeof buffer);
    printf("%04Xh: %s\n", addr, buffer);
}

void debug_print_pc(context_t* ctx)
{
    registers_t regs;

    context_get_registers(ctx, &regs);
    debug_print_addr(ctx, regs.PC);
}
