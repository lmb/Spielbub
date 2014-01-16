#ifndef __SPIELBUB_H__
#define __SPIELBUB_H__

#include <stdbool.h>

typedef struct context context_t;

context_t* context_create(void);
bool context_load_rom(context_t *ctx, const char* filename);
void context_destroy(context_t *ctx);
void context_run(context_t *ctx);

#endif//__SPIELBUB_H__