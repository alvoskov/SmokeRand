#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief SQXOR 64-bit PRNG state.
 */
typedef struct {
    uint32_t entropy;
    uint32_t state[4];
} StormDropState;


static inline uint64_t get_bits_raw(void *state)
{
    StormDropState *obj = state;
    // This variant fails `bspace16_4d` from `full` battery
    obj->entropy += obj->entropy << 16;
    obj->state[0] += obj->state[1] ^ obj->entropy;

/*
    // This variant fails MatrixRank (but not LinearComp) tests
    obj->entropy ^= obj->entropy << 16;            
    obj->state[0] ^= obj->entropy;                 
    obj->entropy ^= (obj->state[1] ^ obj->entropy) >> 5;
*/
    obj->state[1]++;                        
    obj->state[2] ^= obj->entropy;                 
    obj->entropy += obj->entropy << 6;             
    obj->state[3] ^= obj->state[2] ^ obj->entropy;      
    obj->entropy ^= obj->state[0] ^ (obj->entropy >> 9);
    return obj->entropy ^= obj->state[3];          
}                                      

static void *create(const CallerAPI *intf)
{
    StormDropState *obj = intf->malloc(sizeof(StormDropState));
    obj->entropy = intf->get_seed32();
    obj->state[0] = intf->get_seed32();
    obj->state[1] = intf->get_seed32();
    obj->state[2] = intf->get_seed32();
    obj->state[3] = intf->get_seed32();
    return (void *) obj;
}


MAKE_UINT32_PRNG("StormDrop", NULL)



