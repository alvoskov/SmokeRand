/**
 * @file splitmix_shared.c
 * @brief A modified version of SplitMix that is resistant to bad gammas,
 *
 * References:
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief SplitMix PRNG state.
 */
typedef struct {
    uint64_t x;
} SplitMixState;

static inline uint64_t get_bits_raw(void *state)
{
    SplitMixState *obj = state;
    const uint64_t gamma = 0xBBBBBBBBBBBBBBBB;//0x9E3779B97F4A7C15;
    uint64_t z = (obj->x += gamma);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

static void *create(const CallerAPI *intf)
{
    SplitMixState *obj = intf->malloc(sizeof(SplitMixState));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("SplitMix", NULL)
