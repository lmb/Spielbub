#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <sys/select.h>
#include <sys/time.h>

#include "debugger/debug.h"
#include "debugger/commands.h"

bool debug_init(debug_t* dbg)
{
    memset(dbg->commandline, 0, sizeof dbg->commandline);
    dbg->post_exec = &debug_print_pc;
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

void debug_print_pc(context_t* ctx, debug_t* dbg)
{
    registers_t regs;
    char buffer[256] = "";

    context_get_registers(ctx, &regs);
    context_decode_instruction(ctx, regs.PC, buffer, sizeof buffer);

    printf("%04x: %s\n", regs.PC, buffer);
}

void debug_update(context_t* ctx, debug_t *dbg)
{
    static bool write_prompt = true;
    static fd_set input;
    static struct timeval timeout;

    FD_ZERO(&input);
    FD_SET(fileno(stdin), &input);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;

    if (dbg->post_exec != NULL)
    {
        dbg->post_exec(ctx, dbg);
        dbg->post_exec = NULL;
    }

    if (write_prompt) {
        printf("> ");
        fflush(stdout);
        write_prompt = false;
    }

    if (select(fileno(stdin) + 1, &input, NULL, NULL, &timeout) > 0) {
        size_t len = strlen(dbg->commandline);
        size_t rem = sizeof(dbg->commandline) - len - 1;

        if (rem == 0) {
            printf("Command buffer overflow\n");
            dbg->commandline[0] = '\0';
            return;
        }

        fgets(dbg->commandline + len, rem, stdin);

        char* end = strpbrk(dbg->commandline, "\r\n");

        if (end != NULL) {
            ptrdiff_t pos = end - dbg->commandline;
            len = strlen(dbg->commandline);

            dbg->commandline[pos] = '\0';

            execute_command(dbg->commandline, ctx, dbg);

            // Copies trailing \0 as well
            memmove(dbg->commandline, &dbg->commandline[pos + 1], len + pos);
        }

        write_prompt = true;
    }
}

int main(int argc, const char* argv[])
{
    debug_t dbg;
    context_t *ctx;

    if (argc < 2) {
        printf("%s: <rom file>\n", argv[0]);
        return 1;
    }

    if ((ctx = context_create((update_func_t)debug_update, (void*)&dbg)) == NULL)
    {
        printf("Context could not be initialized!\n");
        return 1;
    }

    if(!context_load_rom(ctx, argv[1]))
    {
        printf("Could not load ROM!\n");
        return 1;
    }

    debug_init(&dbg);
    context_set_exec(ctx, STOPPED);
    context_run(ctx);
    debug_free(&dbg);
    context_destroy(ctx);

    return 0;
}
