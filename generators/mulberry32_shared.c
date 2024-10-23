/**
 * @file sqxor32_shared.c
 * @brief Mulberry32 generator by Tommy Ettinger
 * @details Similar to SplitMix: based on 32-bit "discrete Weyl sequence"
 * scrambled by some output function.
 *
 * @copyright The algorithm was suggested by Tommy Ettinger.
 * 
 * Adaptation for SmokeRand:
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Mulberry 32-bit PRNG state.
 */
typedef struct {
    uint32_t w; /**< "Weyl sequence" counter state */
} Mulberry32State;


static uint64_t get_bits(void *state)
{
    Mulberry32State *obj = state;
    uint32_t z = (obj->w += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static void *create(const CallerAPI *intf)
{
    Mulberry32State *obj = intf->malloc(sizeof(Mulberry32State));
    obj->w = intf->get_seed32();
    return (void *) obj;
}


MAKE_UINT32_PRNG("Mulberry32", NULL)
