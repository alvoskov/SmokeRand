/**
 * @file sezgin63_shared.c
 * @brief Implementation of 63-bit LCG with prime modulus.
 * @details Passes SmallCrush and Crush, but systematically gives
 * suspect p-values at BigCrush test N13 (BirthdaySpacings, t = 2).
 *
 * References:
 * - F. Sezgin, T.M. Sezgin. Finding the best portable
 *   congruential random number generators // Computer Physics
 *   Communications. 2013. V. 184. N 8. P. 1889-1897.
 *   https://doi.org/10.1016/j.cpc.2013.03.013.
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
 * @brief 63-bit LCG state.
 */
typedef struct {
    int64_t x; ///< Must be signed!
} Lcg63State;


static uint64_t get_bits(void *state)
{
    static const int64_t m = 9223372036854775783LL; // 2^63 - 25
    static const int64_t a = 3163036175LL; // See Line 4 in Table 1
    static const int64_t b = 2915986895LL;
    static const int64_t c = 2143849158LL;
    Lcg63State *obj = state;    
    obj->x = a * (obj->x % b) - c*(obj->x / b);
    if (obj->x < 0LL) {
        obj->x += m;
    }
    return obj->x >> 31;
}

static void *create(const CallerAPI *intf)
{
    Lcg63State *obj = intf->malloc(sizeof(Lcg63State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT32_PRNG("Sezgin63", NULL)

