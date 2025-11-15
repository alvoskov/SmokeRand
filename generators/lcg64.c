/**
 * @file lcg64.c
 * @brief Just 64-bit LCG that returns the upper 32 bits.
 * The easy to remember multiplier is suggested by George Marsaglia.
 * Slightly better multipliers are present in 
 * @details Slightly better multipliers can be found at:
 *
 * 1. Steele G.L., Vigna S. Computationally easy, spectrally good multipliers
 *    for congruential pseudorandom number generators // Softw Pract Exper. 2022.
 *    V. 52. N. 2. P. 443-458. https://doi.org/10.1002/spe.3030
 * 2. TAOCP2.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/////////////////////////////
///// Marsaglia version /////
/////////////////////////////

static inline uint64_t get_bits_marsaglia_raw(void *state)
{
    Lcg64State *obj = state;
    obj->x = obj->x * 6906969069ULL + 1ULL;
    return obj->x >> 32;
}

MAKE_GET_BITS_WRAPPERS(marsaglia)

/////////////////////////////////////////
///// Version from TAOCP (by Hayes) /////
/////////////////////////////////////////

static inline uint64_t get_bits_taocp_raw(void *state)
{
    Lcg64State *obj = state;
    obj->x = obj->x * 6364136223846793005ULL + 1442695040888963407ULL;
    return obj->x >> 32;
}

MAKE_GET_BITS_WRAPPERS(taocp)

//////////////////////////////////
///// Steele & Vigna version /////
//////////////////////////////////

static inline uint64_t get_bits_steele_raw(void *state)
{
    Lcg64State *obj = state;
    obj->x = obj->x * 0xf1357aea2e62a9c5ULL + 1442695040888963407ULL;
    return obj->x >> 32;
}

MAKE_GET_BITS_WRAPPERS(steele)

//////////////////////
///// Interfaces /////
//////////////////////

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}


static const GeneratorParamVariant gen_list[] = {
    {"",          "Lcg64:Marsaglia", 32, default_create, get_bits_marsaglia, get_sum_marsaglia},
    {"marsaglia", "Lcg64:Marsaglia", 32, default_create, get_bits_marsaglia, get_sum_marsaglia},
    {"taocp",     "Lcg64:TAOCP",     32, default_create, get_bits_taocp,     get_sum_taocp},
    {"steele",    "Lcg64:Steele",    32, default_create, get_bits_steele,    get_sum_steele},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"The x = (ax + c) mod 2^64 LCG that returns the upper 32 bits.\n"
"The next param values are supported:\n"
"  marsaglia - a = 6906969069 (default version)\n"
"  taocp     - a = 6364136223846793005\n"
"  steele    - a = 0xf1357aea2e62a9c5\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
