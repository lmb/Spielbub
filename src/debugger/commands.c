#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "debugger/commands.h"

#define NUM(x) (sizeof(x) / sizeof(x[0]))
#define CEIL(a, b) ((a + (b - 1)) / b)

static void exec_next(const char* args, context_t* ctx, debug_t* dbg);
static void exec_pause(const char* args, context_t* ctx, debug_t* dbg);
static void exec_continue(const char* args, context_t* ctx, debug_t* dbg);
static void exec_frame(const char* args, context_t* ctx, debug_t* dbg);
static void exec_quit(const char* args, context_t* ctx, debug_t* dbg);
static void exec_help(const char* args, context_t* ctx, debug_t* dbg);
static void exec_list(const char* args, context_t* ctx, debug_t* dbg);
static void exec_breakpoint(const char* args, context_t* ctx, debug_t* dbg);
static void exec_traceback(const char* args, context_t* ctx, debug_t* dbg);
static void exec_print(const char* args, context_t* ctx, debug_t* dbg);
static void exec_registers(const char* args, context_t* ctx, debug_t* dbg);
static void exec_viewtiles(const char* args, context_t* ctx, debug_t* dbg);
static void exec_layer(const char* args, context_t* ctx, debug_t* dbg);

static const struct {
    command_t handler;
    const char* verb;
} commands[] = {
    { &exec_next, "next" },
    { &exec_pause, "pause" },
    { &exec_continue, "continue" },
    { &exec_frame, "frame" },
    { &exec_quit, "quit" },
    { &exec_help, "help" },
    { &exec_list, "list" },
    { &exec_breakpoint, "breakpoint" },
    { &exec_traceback, "traceback" },
    { &exec_print, "print" },
    { &exec_registers, "registers" },
    { &exec_viewtiles, "viewtiles" },
    { &exec_layer, "layer" }
};

bool execute_command(const char* command, context_t* ctx, debug_t* dbg)
{
    const char* end = strpbrk(command, " \t");

    size_t verb_len;

    if (end != NULL) {
        verb_len = end - command;
        end += 1;
    } else {
        verb_len = strlen(command);
        end = command + verb_len;
    }

    command_t handler = NULL;

    if (verb_len == 0) {
        handler = dbg->previous_command;
    } else {
        for (size_t i = 0; i < NUM(commands); i++) {
            if (strncmp(commands[i].verb, command, verb_len) == 0) {
                if (handler != NULL) {
                    printf("Ambiguous command\n");
                    return false;
                }

                handler = commands[i].handler;
            }
        }
    }

    dbg->previous_command = handler;

    if (handler == NULL) {
        printf("Invalid command\n");
        return false;
    }

    handler(end, ctx, dbg);

    return true;
}

static void exec_next(const char* args, context_t* ctx, debug_t* dbg)
{
    (void)args;
    (void)dbg;

    context_single_step(ctx);
}

static void exec_pause(const char* args, context_t* ctx, debug_t* dbg)
{
    (void)args;
    (void)dbg;

    printf("Paused.\n");
    context_pause_exec(ctx);
    debug_print_pc(ctx);
}

static void exec_continue(const char* args, context_t* ctx, debug_t* dbg)
{
    (void)args;
    (void)dbg;

    printf("Continuing.\n");
    context_resume_exec(ctx);
}

static void exec_frame(const char* args, context_t* ctx, debug_t* dbg)
{
    (void)args;
    (void)dbg;

    printf("Frame stepping.\n");
    context_frame_step(ctx);
}

static void exec_quit(const char* args, context_t* ctx, debug_t* dbg)
{
    (void)args;
    (void)dbg;

    context_quit(ctx);
}

static void exec_help(const char* args, context_t* ctx, debug_t* dbg)
{
    (void)args;
    (void)ctx;
    (void)dbg;

    printf("The following commands are supported:\n");

    for (size_t i = 0; i < NUM(commands); i++) {
        printf("\t%s\n", commands[i].verb);
    }
}

static void exec_list(const char* args, context_t* ctx, debug_t* dbg)
{
    uint16_t addr;

    (void)dbg;

    if (sscanf(args, "%hx", &addr) != 1) {
        registers_t regs;
        context_get_registers(ctx, &regs);
        addr = regs.PC;
    }

    debug_print_func(ctx, addr);
}

static void exec_breakpoint(const char* args, context_t* ctx, debug_t* dbg)
{
    uint16_t addr;

    (void)dbg;

    if (sscanf(args, "%hx", &addr) != 1) {
        printf("Please specify an address: breakpoint <addr>\n");
        return;
    }

    if (context_add_breakpoint(ctx, addr)) {
        printf("Set breakpoint for 0x%04x\n", addr);
    } else {
        printf("Failed to set breakpoint. Are there too many already?\n");
    }
}

static void exec_traceback(const char* args, context_t* ctx, debug_t* dbg)
{
    uint16_t value;

    (void)args;
    (void)dbg;

    context_reset_traceback(ctx);
    while (context_get_traceback(ctx, &value)) {
        debug_print_addr(ctx, value);
    }
}

static void exec_print(const char* args, context_t* ctx, debug_t* dbg)
{
    uint16_t value, len;

    (void)dbg;

    if (sscanf(args, "%hx:%hx", &value, &len) != 2) {
        len = 1;

        if (sscanf(args, "%hx", &value) != 1) {
            printf("Please specify <addr>(:<len>)\n");
            return;
        }
    }

    uint8_t buffer[len];
    len = context_get_memory(ctx, buffer, value, len);

    for (size_t i = 0; i < len; i++, value++) {
        if (i % 16 == 0) {
            printf("\n[%04x]: ", value);
        }

        printf("%02x ", buffer[i]);
    }

    printf("\n");
}

static void exec_registers(const char* args, context_t* ctx, debug_t* dbg)
{
    registers_t regs;

    (void)args;
    (void)dbg;

    context_get_registers(ctx, &regs);

    printf("   AF = %04Xh, BC = %04Xh, DE = %04Xh, HL = %04Xh\n",
        regs.AF, regs.BC, regs.DE, regs.HL);
    printf("   PC = %04Xh, SP = %04Xh\n", regs.PC, regs.SP);

    char Z, N, C, H;
    uint8_t F = regs.AF & 0xF;

    Z = F & 0x80 ? 'Z' : '-';
    N = F & 0x40 ? 'N' : '-';
    C = F & 0x20 ? 'C' : '-';
    H = F & 0x10 ? 'H' : '-';

    printf ("   Flags: %c%c%c%c\n", Z, N, H, C);
}

static void exec_viewtiles(const char* args, context_t* ctx, debug_t* dbg)
{
    (void)args;
    (void)ctx;

    if (!dbg->show_tiles) {
        dbg->window = graphics_create_window(
            "Tiles",
            (DBG_TILES_PER_ROW) * (TILE_WIDTH + 1),
            CEIL(MAX_TILES, DBG_TILES_PER_ROW) * (TILE_HEIGHT + 1)
        );

        if (dbg->window == NULL) {
            printf("Failed to create window\n");
            return;
        }
    } else {
        graphics_free_window(dbg->window);
        dbg->window = NULL;
    }

    dbg->show_tiles = !dbg->show_tiles;
    
    printf("Toggling tile view.\n");
}

static void exec_layer(const char* args, context_t* ctx, debug_t* dbg)
{
    static const struct {
        const char* name;
        const graphics_layer_t layer;
    } layers[] = {
        { "background", LAYER_BACKGROUND },
        { "window", LAYER_WINDOW },
        { "sprites", LAYER_SPRITES }
    };

    graphics_layer_t layer;
    const char* name = NULL;
    size_t arglen = strlen(args);

    (void)dbg;

    for (size_t i = 0; i < NUM(layers); i++) {
        if (strncmp(layers[i].name, args, arglen) == 0) {
            if (name != NULL) {
                printf("Ambiguous layer, choices are:\n");
                printf("\tbackground, window, sprites\n");
                return;
            }

            name = layers[i].name;
            layer = layers[i].layer;
        }
    }

    if (name == NULL) {
        printf("Invalid layer specified\n");
        return;
    }

    printf("Toggled layer %s\n", name);
    graphics_toggle_debug(ctx, layer);
}
