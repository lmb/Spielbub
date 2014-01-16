#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdarg.h>
#include <stdbool.h>

#define LOG_NUM (10)
#define LOG_LEN (256)

#define log_dbg(x, ...) _log(x, ##__VA_ARGS__)
void _log(const char* str, ...);

void log_init( );
void log_destroy( );
void log_reset( );
bool log_read(char* dst);

#endif//__LOGGING_H__
