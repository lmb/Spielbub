#include <string.h>

#include "probability_list.h"
#include "murmur3.h"

#define SEED1 (0x93498)
#define SEED2 (0x22745)

void pl_init(prob_list_t *pl)
{
    memset(pl, 0, sizeof(*pl));
}

bool pl_add(prob_list_t *pl, uint16_t value)
{
    if (pl->length == PL_MAX_VALUES)
    {
        return false;
    }
    
    uint32_t hash1, hash2;
    
    MurmurHash3_x86_32((const void*)&value, 2, SEED1, (void*)&hash1);
    MurmurHash3_x86_32((const void*)&value, 2, SEED2, (void*)&hash2);
    
    pl->bloom_filter = pl->bloom_filter | hash1 | hash2;
    pl->values[(pl->length)++] = value;
    return true;
}

bool pl_check(const prob_list_t *pl, uint16_t value)
{
    uint32_t hash1, hash2;
    
    MurmurHash3_x86_32((const void*)&value, 2, SEED1, (void*)&hash1);
    MurmurHash3_x86_32((const void*)&value, 2, SEED2, (void*)&hash2);
    
    if ((pl->bloom_filter & hash1) == hash1
        && (pl->bloom_filter & hash2) == hash2)
    {
        // Check for false positive, this is O(n) right now
        unsigned int i;
        
        for (i = 0; i < pl->length; i++)
        {
            if (value == pl->values[i])
                return true;
        }
    }
    
    return false;
}
