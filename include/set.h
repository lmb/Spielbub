#ifndef __SET_H__
#define __SET_H__

#define SET_MAX_VALUES (20)

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t bloom_filter;
    uint16_t values[SET_MAX_VALUES];
    unsigned int length;
} set_t;

void set_init(set_t *set);
bool set_add(set_t *set, uint16_t value);
void set_remove(set_t* set, uint16_t value);
bool set_contains(const set_t *set, uint16_t value);

#endif//__SET_H__
