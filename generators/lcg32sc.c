/**
 * @file lcg32sc.c
 * @brief A 32-bit LCG with a custom scrambler (developed by A.L. Voskov)
 * resembling PCG and PCG-DXSM.
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg32State *obj)
{
    uint32_t out = obj->x ^ (obj->x >> 16);
    out *= 69069U;
    out = out ^ rotl32(out, 7) ^ rotl32(out, 23);
    obj->x = 69069U * obj->x + 12345U;
    return out;
}

static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("LCG32sc", NULL)
