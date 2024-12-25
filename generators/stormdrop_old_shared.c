/**
 * @file stormdrop_shared.c
 * @brief StormDrop pseudorandom number generator.
 * @details It has at least two versions. This is the older one that fails
 * matrix rank tests but not linear complexity tests.
 *
 * References:
 * 1. Wil Parsons. StormDrop is a New 32-Bit PRNG That Passes Statistical Tests
 *    With Efficient Resource Usage
 *    https://medium.com/@wilparsons/stormdrop-is-a-new-32-bit-prng-that-passes-statistical-tests-with-efficient-resource-usage-59b6d6d9c1a8
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief StormDrop PRNG state.
 */
typedef struct {
    uint32_t entropy;
    uint32_t state[4];
} StormDropState;


static inline uint64_t get_bits_raw(void *state)
{
    StormDropState *obj = state;
    // This variant fails MatrixRank (but not LinearComp) tests
    obj->entropy ^= obj->entropy << 16;            
    obj->state[0] ^= obj->entropy;                 
    obj->entropy ^= (obj->state[1] ^ obj->entropy) >> 5;
    // End of variable part
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



