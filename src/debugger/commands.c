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
static void exec_viewtiles(const char* args, context_t* ctx, debug_t* dbg);
static void exec_viewmaps(const char* args, context_t* ctx, debug_t* dbg);
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
    { &exec_viewtiles, "viewtiles" },
    { &exec_viewmaps, "viewmaps" },
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
    static const struct {
        const char* name;
        const size_t offset;
    } registers[] = {
        { "AF", offsetof(registers_t, AF) },
        { "BC", offsetof(registers_t, BC) },
        { "DE", offsetof(registers_t, DE) },
        { "HL", offsetof(registers_t, HL) },
        { "SP", offsetof(registers_t, SP) },
        { "PC", offsetof(registers_t, PC) }
    };

    uint16_t value, len;
    registers_t regs;

    (void)dbg;

    context_get_registers(ctx, &regs);

    for (size_t i = 0; i < NUM(registers); i++) {
        if (strcmp(args, registers[i].name) == 0) {
            value = *((uint16_t*)(((uint8_t*)&regs) + registers[i].offset));
            printf("%s = %04x\n", registers[i].name, value);
            return;
        }
    }

    if (sscanf(args, "%hx:%hx", &value, &len) != 2) {
        printf("Please specify a register <AF|BC|DE|HL|SP|PC> or <addr:len>\n");
        return;
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

static void exec_viewmaps(const char* args, context_t* ctx, debug_t* dbg)
{
    tile_map_t map;

    (void)args;
    (void)dbg;

    context_get_map(ctx, &map, TILE_MAP_LOW);

    // print("Low tile map:\n");
    // debug_print_map(&map);

    // context_get_map(ctx, &map, TILE_MAP_HIGH);

    // printf("High tile map:\n");
    // debug_print_map(&map);
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
