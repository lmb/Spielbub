#include <string.h>

#include "set.h"
#include "murmur3.h"

#define SEED1 (0x93498)
#define SEED2 (0x22745)

#define NUM(x) (sizeof x / sizeof x[0])

static uint32_t add_to_filter(uint32_t filter, uint16_t value);

void set_init(set_t *set)
{
    memset(set, 0, sizeof(*set));
}

bool set_add(set_t *set, uint16_t value)
{
    if (set->length == NUM(set->values))
    {
        return false;
    }

    for (size_t i = 0; i < set->length; i++) {
        if (value == set->values[i]) {
            return true;
        }
    }

    set->bloom_filter = add_to_filter(set->bloom_filter, value);
    set->values[(set->length)++] = value;

    return true;
}

void set_remove(set_t *set, uint16_t value)
{
    for (size_t i = 0; i < set->length; i++) {
        if (value == set->values[i]) {
            memmove(&set->values[i], &set->values[i+1], set->length - i - 1);
            set->length--;
            break;
        }
    }

    set->bloom_filter = 0;
    for (size_t i = 0; i < set->length; i++) {
        set->bloom_filter = add_to_filter(set->bloom_filter, set->values[i]);
    }
}

bool set_contains(const set_t *set, uint16_t value)
{
    if (set->bloom_filter == 0)
        return false;
    
    uint32_t hash1, hash2;
    
    MurmurHash3_x86_32((const void*)&value, 2, SEED1, (void*)&hash1);
    MurmurHash3_x86_32((const void*)&value, 2, SEED2, (void*)&hash2);
    
    if ((set->bloom_filter & hash1) == hash1
        && (set->bloom_filter & hash2) == hash2)
    {
        // Check for false positive, this is O(n) right now
        for (size_t i = 0; i < set->length; i++)
        {
            if (value == set->values[i])
                return true;
        }
    }
    
    return false;
}

static uint32_t add_to_filter(uint32_t filter, uint16_t value)
{
    uint32_t hash1, hash2;
    
    MurmurHash3_x86_32((const void*)&value, 2, SEED1, (void*)&hash1);
    MurmurHash3_x86_32((const void*)&value, 2, SEED2, (void*)&hash2);

    return filter | hash1 | hash2;
}
