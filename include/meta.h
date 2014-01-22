#ifndef __META_H__
#define __META_H__

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char* description;
    int   cycles;
} opcode_meta_t;

extern const opcode_meta_t opcode_meta[];
extern const opcode_meta_t ext_opcode_meta[];

void meta_parse(char *meta, size_t len, const uint8_t* opcode);

#endif//__META_H__
