#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "debugger/commands.h"

#define NUM(x) (sizeof(x) / sizeof(x[0]))

static void exec_next(const char* args, context_t* ctx, debug_t* dbg);
static void exec_pause(const char* args, context_t* ctx, debug_t* dbg);
static void exec_continue(const char* args, context_t* ctx, debug_t* dbg);
static void exec_quit(const char* args, context_t* ctx, debug_t* dbg);
static void exec_help(const char* args, context_t* ctx, debug_t* dbg);
static void exec_list(const char* args, context_t* ctx, debug_t* dbg);
static void exec_breakpoint(const char* args, context_t* ctx, debug_t* dbg);
static void exec_traceback(const char* args, context_t* ctx, debug_t* dbg);
static void exec_print(const char* args, context_t* ctx, debug_t* dbg);

static const struct {
    command_t handler;
    const char* verb;
} commands[] = {
    { &exec_next, "next" },
    { &exec_pause, "pause" },
    { &exec_continue, "continue" },
    { &exec_quit, "quit" },
    { &exec_help, "help" },
    { &exec_list, "list" },
    { &exec_breakpoint, "breakpoint" },
    { &exec_traceback, "traceback" },
    { &exec_print, "print" }
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
    context_single_step(ctx);
    dbg->post_exec = &debug_post_exec_print_pc;
}

static void exec_pause(const char* args, context_t* ctx, debug_t* dbg)
{
    printf("Paused.\n");
    context_pause_exec(ctx);
    debug_post_exec_print_pc(ctx, dbg);
}

static void exec_continue(const char* args, context_t* ctx, debug_t* dbg)
{
    printf("Continuing.\n");
    context_resume_exec(ctx);
}

static void exec_quit(const char* args, context_t* ctx, debug_t* dbg)
{
    context_quit(ctx);
}

static void exec_help(const char* args, context_t* ctx, debug_t* dbg)
{
    printf("The following commands are supported:\n");

    for (size_t i = 0; i < NUM(commands); i++) {
        printf("\t%s\n", commands[i].verb);
    }
}

static void exec_list(const char* args, context_t* ctx, debug_t* dbg)
{
    uint16_t addr;

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

    context_reset_traceback(ctx);
    while (context_get_traceback(ctx, &value)) {
        debug_print_addr(ctx, value);
    }
}

static void exec_print(const char* args, context_t* ctx, debug_t* dbg)
{
    static const struct {
        char* name;
        size_t offset;
    } registers[] = {
        { "AF", offsetof(registers_t, AF) },
        { "BC", offsetof(registers_t, BC) },
        { "DE", offsetof(registers_t, DE) },
        { "HL", offsetof(registers_t, HL) },
        { "SP", offsetof(registers_t, SP) }
    };

    uint16_t value;
    registers_t regs;
    context_get_registers(ctx, &regs);

    for (size_t i = 0; i < NUM(registers); i++) {
        if (strcmp(args, registers[i].name) == 0) {
            value = *((uint16_t*)(((void*)&regs) + registers[i].offset));
            printf("%s = %04x\n", registers[i].name, value);
            return;
        }
    }

    if (sscanf(args, "%hx", &value) != 1) {
        printf("Please specify a register: AF|BC|DE|HL|SP|0xHHHH\n");
        return;
    }

    printf("[%04x]: %02x\n", value, context_get_memory(ctx, value));
}
