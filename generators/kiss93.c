/**
 * @file kiss93.c
 * @brief KISS93 pseudorandom number generator. It passes SmallCrush
 * but fails the LinearComp (r = 29) test in the Crush battery (N72).
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * The KISS93 algorithm is developed by George Marsaglia.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief KISS93 PRNG state.
 */
typedef struct {
    uint32_t lcg;
    uint32_t xs1;
    uint32_t xs2;
} KISS93State;

static inline uint64_t get_bits_raw(void *state)
{
    KISS93State *obj = state;
    // LCG
    obj->lcg = 69069U * obj->lcg + 23606797U;
    // Some LFSR
    uint32_t b = obj->xs1 ^ (obj->xs1 << 17);
    obj->xs1 = (b >> 15) ^ b;
    b = ((obj->xs2 << 18) ^ obj->xs2) & 0x7fffffffU;
    obj->xs2 = (b >> 13) ^ b;
    const uint32_t u = obj->lcg + obj->xs1 + obj->xs2;
    return u;
}


static void *create(const CallerAPI *intf)
{
    // Default seeds: 12345, 6789, 111213
    KISS93State *obj = intf->malloc(sizeof(KISS93State));
    seed64_to_2x32(intf, &obj->lcg, &obj->xs1);
    obj->xs2 = 111213;
    return obj;
}

MAKE_UINT32_PRNG("KISS93", NULL)
