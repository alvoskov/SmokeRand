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
    uint64_t out = obj->x ^ (obj->x >> 32);
    out *= 6906969069U;
    out = out ^ rotl64(out, 17) ^ rotl64(out, 53);
    obj->x += obj->x * obj->x | 0x40000005;
    return out;
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("tf0_64sc2", NULL)
