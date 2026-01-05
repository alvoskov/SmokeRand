/**
 * @file lcg69069.c
 * @brief An implementation of classic 32-bit LCG suggested by G.Marsaglia.
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg32State *obj)
{
    obj->x = 69069u * obj->x + 12345u;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed32();
    return obj;
}


MAKE_UINT32_PRNG("LCG69069", NULL)
