#include "buffers.h"
#include "logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
 * Initialize a circular buffer. Returned pointer must
 * be freed via cb_destroy(). Allocates room for <num> items
 * times <len> bytes.
 */
circular_buffer* cb_init(int num, int len)
{
    circular_buffer *buf = malloc(sizeof(circular_buffer));
    memset(buf, 0, sizeof(circular_buffer));

    buf->buffer = malloc(num * len);
    memset(buf->buffer, 0, num * len);

    buf->len     = len;
    buf->num     = num;

    return buf;
}

void cb_destroy(circular_buffer* buf)
{
    free(buf->buffer);
    free(buf);
}

/*
 * Write to a circular buffer, appending to it and if
 * necessary "pushing out" the oldest entry.
 * Always reads <num> bytes from src.
 */
inline void cb_write(circular_buffer* buf, void* src)
{
    // Pointer to the element to be written.
    // buf->write is the current position of the write "head"
    uint8_t* p = buf->buffer + (buf->write++ * buf->len);

    if (buf->write >= buf->num)
    {
        // Since we post-increment buf->write, wrapping is checked
        // here. In case the write head moves past the
        // end of the buffer, it is reset and buf->wrapped
        // is set to indicate that the buffer now holds
        // the full <num> elements.
        buf->write   = 0;
        buf->wrapped = true;
    }

#if defined(DEBUG)
    // Just to be on the safe side when debugging
    memset(p, 0, buf->len);
#endif
    memcpy(p, src, buf->len);
}

/*
 * Writes a string to the buffer, making sure it is correctly terminated.
 * Maximum string length is 255 characters.
 */
void cb_write_string(circular_buffer* buf, const char* src)
{
    // TODO: Remove intermediate buffer. Assume our user is capable of not
    // shooting himself in the foot.
    unsigned int len = strlen(src);
    char buffer[256];

    assert(len < 256); // TODO: properly check & handle this

#if defined(DEBUG)
    memset(buffer, 0, sizeof(buffer));
#endif

    strncpy(buffer, src, buf->len);
    buffer[255] = '\0';

    cb_write(buf, buffer);
}

/*
 * Resets the read "head".
 */
void cb_reset(circular_buffer* buf)
{
    if (buf->wrapped)
    {
        // Buffer is full. Always read <num> entries,
        // starting at the write head's current position.
        buf->read    = buf->write;
        buf->to_read = buf->num;
    }
    else
    {
        // Buffer is not full. Start from 0 and
        // read up to the write head.
        buf->read    = 0;
        buf->to_read = buf->write;
    }
}

/*
 * Reads from the buffer. dst must be able to hold
 * <len> bytes.
 */
bool cb_read(circular_buffer* buf, uint8_t* dst)
{
    memcpy(dst, buf->buffer + (buf->read++ * buf->len), buf->len);

    if (buf->read >= buf->num)
        buf->read = 0;

    return (--buf->to_read >= 0);
}

