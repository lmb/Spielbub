#include <stdio.h>

#include "context.h"

#include "logging.h"

void context_log(context_t *ctx, const char* str, ...)
{
    char tmp[LOG_LEN];
    va_list va;
    
    tmp[LOG_LEN-1] = '\0';

    va_start(va, str);
    vsnprintf(tmp, LOG_LEN-1, str, va);
    va_end(va);

    cb_write_string(ctx->logs, tmp);
}
