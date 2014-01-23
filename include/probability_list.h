#ifndef __PROBABILITY_LIST_H__
#define __PROBABILITY_LIST_H__

#define PL_MAX_VALUES (20)

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t bloom_filter;
    uint16_t values[PL_MAX_VALUES];
    unsigned int length;
} prob_list_t;

void pl_init(prob_list_t *pl);
bool pl_add(prob_list_t *pl, uint16_t value);
void pl_remove(prob_list_t* pl, uint16_t value);
bool pl_check(const prob_list_t *pl, uint16_t value);

#endif//__PROBABILITY_LIST_H__
