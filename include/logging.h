#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "spielbub.h"

#define LOG_NUM (10)
#define LOG_LEN (256)

#if defined(DEBUG)
#define log_dbg(ctx, msg, ...) context_log(ctx, msg, ##__VA_ARGS__)
#else
#define log_dbg(ctx, msg, ...)
#endif

void context_log(context_t *ctx, const char* str, ...);

#endif//__LOGGING_H__
