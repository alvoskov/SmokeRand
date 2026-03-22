/**
 * @file tf0_32.c
 * @brief A 32-bit modification of tf0_64 PRNG, the generator based on the
 * Klimov-Shamir "crazy" T-function tf0.
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg32State *obj)
{
    obj->x += obj->x * obj->x | 5;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed32();
    return obj;
}


MAKE_UINT32_PRNG("tf0_32", NULL)
