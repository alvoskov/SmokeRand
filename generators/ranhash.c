/**
 * @file ranhash.c
 * @brief `Ranhash` pseudorandom number generator from "Numerical Recipes.
 * The Art of Scientific Computation" (3rd edition). It is a counter
 * based generator that fails only `hamming_distr` test from `default`
 * and `full` batteries.
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
 * @brief Ranhash PRNG state. The generator is taken from
 * "Numerical Recipes" (3rd edition).
 */
typedef struct {
    uint64_t ctr; ///< 64-bit counter
} RanHashState;


static inline uint64_t get_bits_raw(RanHashState *obj)
{
    uint64_t v = obj->ctr++ * 3935559000370003845ULL + 2691343689449507681ULL;
    v ^= v >> 21;
    v ^= v << 37;
    v ^= v >> 4;
    v *= 4768777513237032717ULL;
    v ^= v << 20;
    v ^= v >> 41;
    v ^= v << 5;
    return v;
}


void *create(const CallerAPI *intf)
{
    RanHashState *obj = intf->malloc(sizeof(RanHashState));
    obj->ctr = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("RanHash", NULL)
