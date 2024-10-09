/**
 * @file rrmxmx_shared.c
 * @brief A modified version of SplitMix that is resistant to bad gammas,
 * i.e. increments in the "discrete Weyl sequence".
 * @details
 * References:
 * 1. https://mostlymangling.blogspot.com/2018/07/on-mixing-functions-in-fast-splittable.html
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RC4 PRNG state.
 */
typedef struct {
    uint64_t x;
} RrmxmxState;


static inline uint64_t ror64(uint64_t x, uint64_t r)
{
    return (x << r) | (x >> (64 - r));
}

static uint64_t get_bits(void *state)
{
    RrmxmxState *obj = state;
    static uint64_t const M = 0x9fb21c651e98df25;
    static const uint64_t gamma = 0x9E3779B97F4A7C15;
    uint64_t v = obj->x += gamma; // even obj->x++ is enough for BigCrush
    v ^= ror64(v, 49) ^ ror64(v, 24);
    v *= M;
    v ^= v >> 28;
    v *= M;
    return v ^ v >> 28;
}

static void *create(const CallerAPI *intf)
{
    RrmxmxState *obj = intf->malloc(sizeof(RrmxmxState));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("rrmxmx", NULL)
