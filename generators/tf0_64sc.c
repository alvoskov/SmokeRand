/**
 * @file tf0_64sc.c
 * @brief A scrambled version of tf0_64 generator.
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    const uint32_t out = (uint32_t) (obj->x >> 32);
    obj->x += obj->x * obj->x | 0x40000005;
    return out ^ rotl32(out, 7) ^ rotl32(out, 23);
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT32_PRNG("tf0_64sc", NULL)
