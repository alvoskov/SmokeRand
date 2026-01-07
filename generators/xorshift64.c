/**
 * @file xorshift64.c
 * @brief An optimized version of xorshift64 generator suggested by S.Vigna.
 * @details Behaves slightly better in birthday spacings/collision over tests
 * than the original XSH (xorshift64) with [13,17,43] parameters. And the
 * derived xorshift64* generator behaves better than `RanQ1` from "Numerical
 * Recipes" (doesn't fail birthday spacings/collision over tests in lower bits).
 *
 * References:
 * 
 * 1. Vigna S. An Experimental Exploration of Marsaglia's xorshift Generators,
 *    Scrambled // ACM Trans. Math. Softw. 2016. V. 42. N 4. Article ID 30.
 *    https://doi.org/10.1145/2845077
 *
 * @copyright The original xorshift64 generator was invented by G.Marsaglia.
 * The xorshift64* modiciation with non-linear output was suggesed by S.Vigna.
 *
 * Thread-safe reentrant reimplementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift64* generator state.
 */
typedef struct {
    uint64_t x; ///< xorshift64 state
} Xorshift64State;


static inline uint64_t get_bits_raw(Xorshift64State *obj)
{
    obj->x ^= obj->x >> 12;
    obj->x ^= obj->x << 25;
    obj->x ^= obj->x >> 27;
    return obj->x;
}


void *create(const CallerAPI *intf)
{
    Xorshift64State *obj = intf->malloc(sizeof(Xorshift64State));
    do {
        obj->x = intf->get_seed64();
    } while (obj->x == 0);
    (void) get_bits_raw(obj);
    return obj;
}


MAKE_UINT64_PRNG("xorshift64", NULL)
