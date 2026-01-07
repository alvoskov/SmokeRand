/**
 * @file xorshift64st.c
 * @brief `Ranq1` pseudorandom number generator from "Numerical Recipes.
 * The Art of Scientific Computation" (3rd edition). It is a modification
 * of classical xorshift64* PRNG. Its lower bits have a low linear
 * complexity. It also fails the `bspace32_2d` test and the 64-bit birthday
 * paradox test.
 * @copyright The original xorshift64 generator was invented by G.Marsaglia.
 * The xorshift64* modiciation with non-linear output was suggesed by S.Vigna.
 * The `Ranq1` variant was suggested by the authors of "Numerical Recipes".
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
 * @brief RanQ1 PRNG state. The generator is taken
 * from "Numerical Recipes" (3rd edition).
 */
typedef struct {
    uint64_t v; ///< xorshift64 state
} Xorshift64StState;


static inline uint64_t get_bits_raw(Xorshift64StState *obj)
{
    obj->v ^= obj->v >> 12;
    obj->v ^= obj->v << 25;
    obj->v ^= obj->v >> 27;
    return obj->v * 2685821657736338717ULL;
}


void *create(const CallerAPI *intf)
{
    Xorshift64StState *obj = intf->malloc(sizeof(Xorshift64StState));
    do {
        obj->v = intf->get_seed64();
    } while (obj->v == 0);
    (void) get_bits_raw(obj);
    return obj;
}


MAKE_UINT64_PRNG("xorshift64*", NULL)
