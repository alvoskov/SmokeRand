/**
 * @file lcg64sc2.c
 * @brief LCG64: scrambled version.
 * @details PractRand 0.94: >= 1 TiB, `full`
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    uint32_t out = (uint32_t) (obj->x >> 32);
    out = (out >> 16) ^ out;
    out = 69069U * out;
    out = out ^ rotl32(out, 7) ^ rotl32(out, 23);
    obj->x = obj->x * 6906969069U + 12345U;
    return out;
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("LCG64SC2", NULL)
