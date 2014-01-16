#include <stdio.h>
#include <stdarg.h>

#include "spielbub.h"

int main(int argc, const char* argv[])
{
    context_t *ctx;

    if (argc < 2) {
        printf("%s: <rom file>\n", argv[0]);
        return 1;
    }

    if ((ctx = context_create()) == NULL)
    {
        printf("Context could not be initialized!\n");
        return 1;
    }

    if(!context_load_rom(ctx, argv[1]))
    {
        printf("Could not load ROM!\n");
        return 1;
    }

    context_run(ctx);
    context_destroy(ctx);

    return 0;
}
