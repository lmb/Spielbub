#include "memory.h"
#include "cpu.h"
#include "buffers.h"
#include "logging.h"

#include "context.h"
//#include "console.h"
#include "meta.h"
#include "hardware.h"

#include <stdio.h>
#include <stdarg.h>
#include <SDL/SDL.h>

#define MEM_DUMP_ROWS (8)
#define MEM_DUMP_COLS (8)

#define OPCODES_X (0)
#define FLAGS_X (22)
#define MEMORY_X (31)
#define REGISTERS_X (56)

#if defined(DEBUG)

/*
 * This dumps a certain amount of memory, specified by rows and columns.
 * x and y are the x and y coordinates to output the memdump to.
 */
/*void dump_mem(unsigned int addr, int n_cols, int n_rows, int x, int y)
{
    int start_addr = (int)(addr / n_cols) * n_cols;
    int end_addr   = start_addr + (n_rows * n_cols);
    int total, cols, rows, count = 0;

    if (end_addr  >= 0xFFFF) end_addr = 0xFFFF;
    total = end_addr - start_addr + 1;

    write_c("0x%04X - 0x%04X", x, y++, start_addr, end_addr);

    for (rows = 0; rows < n_rows; rows++)
    {
        for (cols = 0; cols < n_cols; cols++)
        {
            uint8_t* b = mem_address(start_addr + count);

            if (start_addr + count == addr)
            {
                SetConsoleTextAttribute(_stdout, FOREGROUND_RED);
                write_c("%02x", x + (cols * 3), y + rows, *b);
                SetConsoleTextAttribute(_stdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
            else
                write_c("%02x", x + (cols * 3), y + rows, *b);

            if (++count >= total) goto end;
        }
    }
    end: ;;
}*/

/*
 * Emulator debugging "menu". Unholy mess.
 */
/*void debug()
{
    char inbuf[256] = "", inbuf2[256] = "";
    char log[LOG_LEN];
    int inint, i, pc, memptr = 0x100;

    // Compiled as windows application, no
    // default console.
    AllocConsole();

    for (;;)
    {
        // --- Commands
        if (strcmp(inbuf, "q") == 0)
        {
            // Exit program, big surprise
            break;
        }
        else if (strcmp(inbuf, "s") == 0)
        {
            // Execute one step
            run(0);
            // Set the memdump window to the current
            // program counter.
            memptr = _PC;
        }
        else if (strcmp(inbuf, "r") == 0)
        {
            // Run the emulator. Exits when ESC is
            // pressed in the SDL window.
            run(-1);
            memptr = _PC;
        }
        else if (sscanf(inbuf, "b %x", &inint) == 1)
        {
            // Sets breakpoint to specified memory address.
            // Only breaks when BP == PC, breaks before
            // execution of the breakpoint.
            log_dbg("Set breakpoint to 0x%04X", inint);
            BP = inint;

            // TODO Add support for ISR breakpoints, e.g b vblank
        }
        else if (sscanf(inbuf, "m %x", &inint) == 1)
        {
            // Set the memory dump address
            memptr = inint;
        }

        // List of recently executed opcodes
        write_c("%*s", OPCODES_X, 0, 20, "Opcodes");
        write_c("--------------------", OPCODES_X, 1);

        i = 0;
        cb_reset(context->trace);
        while(cb_read(context->trace, (void*)&pc))
        {
            uint8_t *opcode = mem_address(pc);
            meta_parse(inbuf, opcode);

            clear_rect_c(OPCODES_X, i + 2, 20);
            write_c("%04X %15s", OPCODES_X, i++ + 2, pc, inbuf);
        }

        // Show the next opcode to be executed
        meta_parse(inbuf, mem_address(_PC));
        clear_rect_c(OPCODES_X, i + 2, 20);
        write_c("%04X %15s", OPCODES_X, i + 2, _PC, inbuf);

        // --- Flags & registers
        write_c("  Flags",  FLAGS_X, 0);
        write_c("-------",  FLAGS_X, 1);
        // Last opcode performed a subtraction?
        write_c("N     %d", FLAGS_X, 2, GET_N());
        // Result of last opcode is zero?
        write_c("Z     %d", FLAGS_X, 3, GET_Z());
        // Half-carry occured? (bit 4)
        write_c("H     %d", FLAGS_X, 4, GET_H());
        // Carry occured?
        write_c("C     %d", FLAGS_X, 5, GET_C());

        // Program counter and stack pointer
        write_c("PC % 4x",  FLAGS_X, 7, _PC);
        write_c("SP % 4x",  FLAGS_X, 8, _SP);

        // --- Log messages
        i = 0;
        log_reset();
        while (log_read(log))
        {
            clear_c(21 + i);
            write_c("%s", 0, 21 + i++, log);
        }

        // --- Memory dump
        write_c("Memory", MEMORY_X, 0);
        write_c("-----------------------", MEMORY_X, 1);

        dump_mem(memptr, 8, 8, MEMORY_X, 2);

        // --- Registers
        write_c("      Registers", REGISTERS_X, 0);
        write_c("---------------", REGISTERS_X, 1);
        write_c("AF %04X", REGISTERS_X, 2, _AF); write_c("BC %04X", REGISTERS_X + 8, 2, _BC);
        write_c("DE %04X", REGISTERS_X, 3, _DE); write_c("HL %04X", REGISTERS_X + 8, 3, _HL);

        // --- Stack
        write_c("Stack", REGISTERS_X, 5);
        write_c("-----------------", REGISTERS_X, 6);
        dump_mem(_SP, 6, 2, REGISTERS_X, 7);

        // --- Command input
        clear_c(24);
        write_c("> ", 0, 24);
        while (!read_c(inbuf, sizeof(inbuf)))
        {
            // Prevent SDL window freeze
            SDL_PumpEvents();
            SDL_Delay(10);
        }
    }
}*/
#endif

int main(int argc, char* argv[])
{
    context_t ctx;

    if (argc < 2) {
    	printf("%s: <rom file>\n", argv[0]);
    	return 1;
    }
    
    char *rom_file = argv[1];

    log_init();

    // TODO: Avoid crashing when ROM can not be loaded
    if (!context_init(&ctx))
    {
        printf("Context could not be initialized!");
        return 0;
    }
    
    mem_load_rom(ctx.mem, rom_file);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        log_dbg("SDL init failed: %s", SDL_GetError());
        return -1;
    }
    atexit(SDL_Quit);

    context_run(&ctx);

    context_destroy(&ctx);
    log_destroy();

    return 0;
}
