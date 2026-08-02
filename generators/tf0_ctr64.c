/**
 * @file tf0_ctr64.c
 * @brief 
 * @details
 *
 * - https://github.com/tommyettinger/PractRand-With-Junk/blob/master/src/RNGs/other/mult.cpp
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    uint64_t x = obj->x;
    obj->x += 5555555555555555555U;
    x += x * x | 999911U;
    x ^= x >> 29;
    x += x * x | 119119U;
    x ^= x >> 27;
    return x;
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("tf0_ctr64", NULL)
