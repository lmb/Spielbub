#ifndef __META_H__
#define __META_H__

#include <stdint.h>

#if defined(DEBUG)
typedef struct {
    char* description;
    int   cycles;
} opcode_meta_t;
#else
typedef struct {
    int cycles;
} opcode_meta_t;
#endif

extern const opcode_meta_t opcode_meta[];
extern const opcode_meta_t ext_opcode_meta[];

void meta_parse(char *meta, const uint8_t* opcode);

#endif//__META_H__
