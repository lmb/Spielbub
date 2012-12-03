#include "console.h"

#include <stdio.h>
#include <stdint.h>

#include "vs/compat.h"

HANDLE _stdout;
HANDLE _stdin;

/*
 * Write a line to console. Filters the arguments through
 * vsprintf().
 */
void write_c(const char* str, int x, int y, ...)
{
    COORD c;
    char buffer[256];
    int out;
    va_list va;

    if (!_stdout)
    {
        // TODO move this to a separate function
        _stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    c.X = (int16_t)x; c.Y = (int16_t)y;
    SetConsoleCursorPosition(_stdout, c);

    va_start(va, y);
    // TODO this is buffer overflow heaven.
    vsprintf(buffer, str, va);
    va_end(va);

    WriteConsole(_stdout, buffer, strlen(buffer), &out, NULL);
}

bool read_c(char *inbuf, int len)
{
    INPUT_RECORD ir_buf[128];
    int i, read;
    char* p;

    if (!_stdin)
    {
        _stdin = GetStdHandle(STD_INPUT_HANDLE);
        //SetConsoleMode(_stdin, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    }

    // Using peek console to make it non-blocking. Pretty crude,
    // (almost) works.
    if (PeekConsoleInput(_stdin, ir_buf, 128, &read) != 0)
    {
        for (i = 0; i < read; i++)
        {
            if (
                ir_buf[i].EventType == KEY_EVENT &&
                ir_buf[i].Event.KeyEvent.uChar.AsciiChar == '\r'
            ) {
                ReadConsole(_stdin, inbuf, len-1, &read, NULL);
                p = strchr(inbuf, '\r'); *p = '\0';

                return true;
            }
        }
    }

    return false;
}

/*
 * Clear a rectangle, as indicated by x, y, and len.
 * Simply overwrites it with spaces.
 */
void clear_rect_c(int x, int y, int len)
{
    char tmp[81];
    len = (len > 80) ? 80 : len;
    sprintf(tmp, "%*s", len, "");

    write_c(tmp, x, y);
}

/*
 * Clear a complete line.
 */
void clear_c(int y)
{
    clear_rect_c(0, y, 79);
}
