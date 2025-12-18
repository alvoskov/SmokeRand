/**
 * @file mwc16_32.c
 * @brief
 * @details
 * smokerand brief generators/mwc16_32.dll --testid=17
 * failure
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
    Lcg32State *obj = state;
    uint16_t x_lo = (uint16_t) (obj->x & 0xFFFF);
    uint16_t x_hi = (uint16_t) (obj->x >> 16);
//    obj->x = (uint32_t)63885 * x_lo + x_hi;
//  Causes a glitch in GCC 10 (some UB during optimization?)
    obj->x = 63885U * x_lo + x_hi; 
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    const uint32_t seed0 = intf->get_seed32();
    obj->x = (seed0 & 0xFFFF) | ((uint32_t)1U << 16U);
    return obj;
}


MAKE_UINT32_PRNG("Mwc1632", NULL)
