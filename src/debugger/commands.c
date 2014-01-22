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

static const struct {
    command_t handler;
    const char* verb;
} commands[] = {
    { &exec_next, "next" },
    { &exec_pause, "pause" },
    { &exec_continue, "continue" },
    { &exec_quit, "quit" },
    { &exec_help, "help" }
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
    context_set_exec(ctx, SINGLE_STEPPING);
    dbg->post_exec = &debug_print_pc;
}

static void exec_pause(const char* args, context_t* ctx, debug_t* dbg)
{
    printf("Paused.\n");
    context_set_exec(ctx, STOPPED);
    debug_print_pc(ctx, dbg);
}

static void exec_continue(const char* args, context_t* ctx, debug_t* dbg)
{
    printf("Continuing.\n");
    context_set_exec(ctx, RUNNING);
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
