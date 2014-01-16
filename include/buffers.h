#ifndef __BUFFERS_H__
#define __BUFFERS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint8_t* buffer;
    unsigned int write;
    unsigned int read;
    unsigned int len, num;
    int to_read;
    bool wrapped;
} circular_buffer;

circular_buffer* cb_init(size_t num, size_t len);
void cb_destroy(circular_buffer* buf);
void cb_write(circular_buffer* buf, void* src);
void cb_write_string(circular_buffer* buf, const char* src);
void cb_reset(circular_buffer* buf);
bool cb_read(circular_buffer* buf, uint8_t* dst);

#endif//__BUFFERS_H__
