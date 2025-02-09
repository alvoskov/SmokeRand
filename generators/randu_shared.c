/**
 * @file randu_shared.c
 * @brief An implementation of RANDU - low-quality 32-bit LCG.
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
    Lcg32State *obj = state;
    obj->x = 65539 * obj->x + 12345;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed64() >> 32;
    return (void *) obj;
}


MAKE_UINT32_PRNG("RANDU", NULL)
