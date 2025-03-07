/**
 * @file rrmxmx.c
 * @brief A modified version of SplitMix that is resistant to bad gammas,
 * i.e. increments in the "discrete Weyl sequence".
 * @details
 * References:
 * 1. https://mostlymangling.blogspot.com/2018/07/on-mixing-functions-in-fast-splittable.html
 *
 * @copyright The rrmxmx algorithm was suggested by Pelle Evensen.
 *
 * Adaptation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RRMXMX PRNG state.
 */
typedef struct {
    uint64_t x;
} RrmxmxState;


static inline uint64_t get_bits_raw(void *state)
{
    RrmxmxState *obj = state;
    static uint64_t const M = 0x9fb21c651e98df25;
    static const uint64_t gamma = 0x9E3779B97F4A7C15;
    uint64_t v = obj->x += gamma; // even obj->x++ is enough for BigCrush
    v ^= rotr64(v, 49) ^ rotr64(v, 24);
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
