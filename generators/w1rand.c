/**
 * @file w1rand.c
 * @brief A simple wyrand-style PRNG.
 * @details Developed by wangyi-fudan as a modification of wyrand with sligtly
 * improved performance.
 *
 * 1. https://github.com/wangyi-fudan/wyhash
 * 2. https://github.com/lemire/testingRNG/issues/28
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} W1RandState;

static inline uint64_t get_bits_raw(W1RandState *obj)
{
    static const uint64_t c = 0xd07ebc63274654c7ull;
    uint64_t hi, lo;
    obj->x += c;
    lo = unsigned_mul128(obj->x, obj->x ^ c, &hi);
    return lo ^ hi;
}

static void *create(const CallerAPI *intf)
{
    W1RandState *obj = intf->malloc(sizeof(W1RandState));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("W1Rand", NULL)
