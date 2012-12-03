#include "logging.h"
#include "buffers.h"

#include <stdio.h>

circular_buffer* log_buffer;

void log_init()
{
    log_buffer = cb_init(LOG_NUM, LOG_LEN);
}

void log_destroy()
{
    cb_destroy(log_buffer);
}

void log_reset()
{
    cb_reset(log_buffer);
}

bool log_read(char* dst)
{
    return cb_read(log_buffer, (uint8_t*)dst);
}

void _log(const char* str, ...)
{
    char tmp[LOG_LEN];
    va_list va;
    
    tmp[LOG_LEN-1] = '\0';

    va_start(va, str);
    vsnprintf(tmp, LOG_LEN-1, str, va);
    va_end(va);

    cb_write_string(log_buffer, tmp);
}
