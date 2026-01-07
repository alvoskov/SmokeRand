/**
 * @file stormdrop.c
 * @brief StormDrop pseudorandom number generator.
 * @details It has at least two versions. The next two ones are implemented
 * in this module:
 *
 * - `--param=new`: the newer one that fails `bspace16_4d` test
 *   from `full` battery.
 * - `--param=old`: the older one that fails matrix rank tests but not linear
 *   complexity tests.
 *
 * References:
 *
 * 1. Wil Parsons. StormDrop is a New 32-Bit PRNG That Passes Statistical Tests
 *    With Efficient Resource Usage
 *    https://medium.com/@wilparsons/stormdrop-is-a-new-32-bit-prng-that-passes-statistical-tests-with-efficient-resource-usage-59b6d6d9c1a8
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
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

/////////////////////////////
///// The newer variant /////
/////////////////////////////

static inline uint64_t get_bits_new_raw(StormDropState *obj)
{
    // This variant fails `bspace16_4d` from `full` battery
    obj->entropy += obj->entropy << 16;
    obj->state[0] += obj->state[1] ^ obj->entropy;
    // End of variable part
    obj->state[1]++;                        
    obj->state[2] ^= obj->entropy;                 
    obj->entropy += obj->entropy << 6;             
    obj->state[3] ^= obj->state[2] ^ obj->entropy;      
    obj->entropy ^= obj->state[0] ^ (obj->entropy >> 9);
    return obj->entropy ^= obj->state[3];          
}

MAKE_GET_BITS_WRAPPERS(new)

/////////////////////////////
///// The older variant /////
/////////////////////////////

static inline uint64_t get_bits_old_raw(StormDropState *obj)
{
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

MAKE_GET_BITS_WRAPPERS(old)

//////////////////////
///// Interfaces /////
//////////////////////

static void *create(const CallerAPI *intf)
{
    StormDropState *obj = intf->malloc(sizeof(StormDropState));
    obj->entropy = intf->get_seed32();
    seeds_to_array_u32(intf, obj->state, 4);
    return obj;
}


static const GeneratorParamVariant gen_list[] = {
    {"",    "StormDrop:new", 32, default_create, get_bits_new, get_sum_new},
    {"new", "StormDrop:new", 32, default_create, get_bits_new, get_sum_new},
    {"old", "StormDrop:old", 32, default_create, get_bits_old, get_sum_old},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"StormDrop is an experimental chaotic generator with a linear part (counter)\n"
"The next param values are supported:\n"
"  new - Newer version (default)\n"
"  old - Older version\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
/*
    const char *param = intf->get_param();
    gi->description = NULL;
    gi->create = default_create;
    gi->free = default_free;
    gi->nbits = 32;
    gi->self_test = NULL;
    gi->parent = NULL;
    if (!intf->strcmp(param, "new") || !intf->strcmp(param, "")) {
        gi->name = "StormDrop:new";
        gi->get_bits = get_bits_new;
        gi->get_sum = get_sum_new;
    } else if (!intf->strcmp(param, "old")) {
        gi->name = "StormDrop:old";
        gi->get_bits = get_bits_old;
        gi->get_sum = get_sum_old;
    } else {
        gi->name = "SuperDuper:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
*/
}
