/**
 * @file weyl.c
 * @brief Just 64-bit LCG that returns the upper 32 bits.
 * The easy to remember multiplier is suggested by George Marsaglia.
 * Slightly better multipliers are present in 
 * @details Slightly better multipliers can be found at:
 *
 * 1. Steele G.L., Vigna S. Computationally easy, spectrally good multipliers
 *    for congruential pseudorandom number generators // Softw Pract Exper. 2022.
 *    V. 52. N. 2. P. 443-458. https://doi.org/10.1002/spe.3030
 * 2. TAOCP2.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
    Lcg64State *obj = state;
    obj->x += 0x9E3779B97F4A7C15;
    return obj->x >> 32;
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT32_PRNG("Weyl", NULL)
