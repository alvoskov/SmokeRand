/**
 * @file ranq2.c
 * @brief `Ranq2` pseudorandom number generator from "Numerical Recipes.
 * The Art of Scientific Computation" (3rd edition). It is a combined
 * generator resembling KISS or SuperDuper. However it fails the matrix rank
 * (but not linear complexity) test from `default` and `full` SmokeRand
 * test batteries. It also rapidly fails PractRand 0.94.
 * @copyright The algorithm is suggested by the authors of "Numerical
 * Recipes".
 *
 * Thread-safe reimplementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RanQ2 PRNG state. It is a combination of xorshift64 LFSR and
 * MWC (multiply-with-carry) generator with 64-bit state. The generator
 * is taken from "Numerical Recipes".
 */
typedef struct {
    uint64_t v; ///< xorshift64 state
    uint64_t w; ///< MWC state
} RanQ2State;


static inline uint64_t get_bits_raw(RanQ2State *obj)
{
    obj->v ^= obj->v >> 17;
    obj->v ^= obj->v << 31;
    obj->v ^= obj->v >> 8;
    obj->w = 4294957665ULL * (obj->w & 0xFFFFFFFF) + (obj->w >> 32);
    return obj->w ^ obj->v;
}


void *create(const CallerAPI *intf)
{
    RanQ2State *obj = intf->malloc(sizeof(RanQ2State));
    obj->v = intf->get_seed64();
    if (obj->v == 0) {
        obj->v = 4101842887655102017ULL;
    }
    obj->w = intf->get_seed32() | (1ull << 32);
    return obj;
}

MAKE_UINT64_PRNG("RanQ2", NULL)
