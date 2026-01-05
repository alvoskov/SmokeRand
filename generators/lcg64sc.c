/**
 * @file lcg64sc.c
 * @brief A 64-bit LCG with a custom scrambler (developed by A.L. Voskov)
 * resembling PCG and PCG-DXSM.
 *
 * Passes PractRand 0.94 at least up to 2 TiB, SmokeRand `express`, `brief`,
 * `default`, `full` batteries. TestU01 SmallCrush/Crush/BigCrush passed.
 * For BigCrush: +HI/+LO.
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
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
    obj->x = 6906969069U * obj->x + 12345U;
    return out;
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("LCG64sc", NULL)
