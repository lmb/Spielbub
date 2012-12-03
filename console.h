#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>
#include <stdarg.h>

extern HANDLE _stdout;
extern HANDLE _stdin;

void write_c(const char* str, int x, int y, ...);
void hex_dump(const char* src, int x, int y, int cols, int rows);
void clear_c(int y);
void clear_rect_c(int x, int y, int len);
bool read_c(char *inbuf, int len);

#endif//__CONSOLE_H__
