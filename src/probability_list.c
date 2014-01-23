#include <string.h>

#include "probability_list.h"
#include "murmur3.h"

#define SEED1 (0x93498)
#define SEED2 (0x22745)

#define NUM(x) (sizeof x / sizeof x[0])

static uint32_t add_to_filter(uint32_t filter, uint16_t value);

void pl_init(prob_list_t *pl)
{
    memset(pl, 0, sizeof(*pl));
}

bool pl_add(prob_list_t *pl, uint16_t value)
{
    if (pl->length == NUM(pl->values))
    {
        return false;
    }

    for (size_t i = 0; i < pl->length; i++) {
        if (value == pl->values[i]) {
            return true;
        }
    }

    pl->bloom_filter = add_to_filter(pl->bloom_filter, value);
    pl->values[(pl->length)++] = value;

    return true;
}

void pl_remove(prob_list_t* pl, uint16_t value)
{
    for (size_t i = 0; i < pl->length; i++) {
        if (value == pl->values[i]) {
            memmove(&pl->values[i], &pl->values[i+1], pl->length - i - 1);
            pl->length--;
            break;
        }
    }

    pl->bloom_filter = 0;
    for (size_t i = 0; i < pl->length; i++) {
        pl->bloom_filter = add_to_filter(pl->bloom_filter, pl->values[i]);
    }
}

bool pl_check(const prob_list_t *pl, uint16_t value)
{
    if (pl->bloom_filter == 0)
        return false;
    
    uint32_t hash1, hash2;
    
    MurmurHash3_x86_32((const void*)&value, 2, SEED1, (void*)&hash1);
    MurmurHash3_x86_32((const void*)&value, 2, SEED2, (void*)&hash2);
    
    if ((pl->bloom_filter & hash1) == hash1
        && (pl->bloom_filter & hash2) == hash2)
    {
        // Check for false positive, this is O(n) right now
        for (size_t i = 0; i < pl->length; i++)
        {
            if (value == pl->values[i])
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
