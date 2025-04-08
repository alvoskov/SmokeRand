/**
 * @file msws.c
 * @brief Implementatio of "Middle-Square Weyl Sequence PRNG" 
 * @details Passes SmallCrush, Crush, ??? and ???.
 * References:
 *
 * 1. Bernard Widynski Middle-Square Weyl Sequence RNG
 *    https://arxiv.org/abs/1704.00358
 * 2. https://news.ycombinator.com/item?id=39733685
 *
 * It uses 64-bit LCG instead of "discrete Weyl sequence", the multiplier
 * quality was tested by Knuth's spectral test and requirements from TAOCP2
 * for MCGs (see section 3.2.1.2).
 *
 * PractRand 0.94: >=1 TiB, 0.41 cpb
 *
 * @copyright MSWS algorithm is developed by Bernard Widynski.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Middle-square Weyl sequence PRNG state.
 */
typedef struct {
    uint64_t x; ///< Buffer for output function
    uint64_t mcg; ///< 64-bit MCG generator
} Msws64State;


static inline uint64_t get_bits_raw(void *state)
{
    static const uint64_t s = 0xe9acc0f334e93bd5ull;
    Msws64State *obj = state;
    obj->x = obj->x * obj->x + (obj->mcg *= s);
    obj->x = (obj->x >> 32) | (obj->x << 32);
    return obj->x ^ obj->mcg;
}


static void *create(const CallerAPI *intf)
{
    Msws64State *obj = intf->malloc(sizeof(Msws64State));
    obj->x   = intf->get_seed64();
    obj->mcg = intf->get_seed64() | 0x1;
    return (void *) obj;
}


MAKE_UINT64_PRNG("Msws64", NULL)
