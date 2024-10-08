/**
 * @file lcg69069_shared.c
 * @brief An implementation of classic 32-bit LCG suggested by G.Marsaglia.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 *
 * In scientific publications which used this software, a reference to it
 * would be appreciated.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 32-bit LCG state.
 */
typedef struct {
    uint32_t x;
} Lcg32State;


static uint32_t get_bits32(void *state)
{
    Lcg32State *obj = state;
    obj->x = 69069 * obj->x + 12345;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed64() >> 32;
    return (void *) obj;
}


MAKE_UINT32_PRNG("LCG69069", NULL)
