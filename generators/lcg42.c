/**
 * @file lcg42.c
 * @brief Just 42-bit LCG that returns the upper 32 bits.
 * @details Taken from [1] (Chapter 17.3):
 *
 * 1. Demidovich B.P., Maron I.A. Computational Mathematics.
 *    Mir Publishers, 1981.
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define P42_MASK 0x3FFFFFFFFFF

static inline uint64_t get_bits_raw(void *state)
{
    Lcg64State *obj = state;
    obj->x = (obj->x * 762939453125ull ) & P42_MASK;
    return obj->x >> 10;
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = (intf->get_seed64() & P42_MASK) | 0x1;
    return (void *) obj;
}

MAKE_UINT32_PRNG("LCG42", NULL)
