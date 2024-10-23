/**
 * @file lcg64_shared.c
 * @brief Just 64-bit LCG that returns the upper 32 bits.
 * The easy to remember multiplier is suggested by George Marsaglia.
 * Slightly better multipliers are present in https://doi.org/10.1002/spe.303.
 * @details
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} Lcg64State;

static inline uint64_t get_bits_raw(void *state)
{
    Lcg64State *obj = state;
    obj->x = obj->x * 6906969069 + 1;
    return obj->x >> 32;
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT32_PRNG("LCG64", NULL)
