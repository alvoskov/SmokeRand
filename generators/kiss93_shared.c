/**
 * @file kiss93_shared.c
 * @brief KISS93 pseudorandom number generator. It passes SmallCrush
 * but fails the LinearComp (r = 29) test in the Crush battery (N72).
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * @copyright The KISS93 algorithm is developed by George Marsaglia.
 *
 * Implementation for TestU01-threads:
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief KISS93 PRNG state.
 */
typedef struct {
    uint32_t S1;
    uint32_t S2;
    uint32_t S3;
} KISS93State;

static uint64_t get_bits(void *state)
{
    KISS93State *obj = state;
    obj->S1 = 69069 * obj->S1 + 23606797;
    uint32_t b = obj->S2 ^ (obj->S2 << 17);
    obj->S2 = (b >> 15) ^ b;
    b = ((obj->S3 << 18) ^ obj->S3) & 0x7fffffffU;
    obj->S3 = (b >> 13) ^ b;
    uint32_t u = obj->S1 + obj->S2 + obj->S3;
    return u;
}


static void *create(const CallerAPI *intf)
{
    KISS93State *obj = intf->malloc(sizeof(KISS93State));
    obj->S1 = 12345;
    obj->S2 = 6789;
    obj->S3 = (uint32_t) intf->get_seed64();// Default is 111213;
    return (void *) obj;
}

MAKE_UINT32_PRNG("KISS93", NULL)
