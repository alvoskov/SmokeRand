/**
 * @file sqxor32_shared.c
 * @brief 32-bit modification of SplitMix (mainly for SmokeRand testing)
 * @details 
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
 * @brief SQXOR 32-bit PRNG state.
 */
typedef struct {
    uint32_t w; /**< "Weyl sequence" counter state */
} SplitMix32State;


static uint64_t get_bits(void *state)
{
    SplitMix32State *obj = state;
    const uint32_t c = 0x9E3779B9;
    uint32_t s = (obj->w += c);
    s ^= s >> 16;
    s *= 0x85ebca6b;
    s ^= s >> 13;
    s *= 0xc2b2ae35;
    s ^= s >> 16;
    return s;
}

static void *create(const CallerAPI *intf)
{
    SplitMix32State *obj = intf->malloc(sizeof(SplitMix32State));
    obj->w = intf->get_seed32();
    return (void *) obj;
}


MAKE_UINT32_PRNG("SplitMix32", NULL)
