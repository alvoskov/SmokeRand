/**
 * @file sqxor32.c
 * @brief PRNG inspired by the Von Neumann middle squares method and
 * its modification by B.Widynski. This version has reduced 32-bit
 * state and reduced period (2^{32}).
 * @details "Weyl sequence" variant passes SmallCrush but fails
 * the next tests in Crush:
 * - 76  LongestHeadRun, r = 0          1.6e-10
 * - 78  PeriodsInStrings, r = 0         1.1e-8
 * It also fails PractRand after generating 32GiB of data.
 *
 * "Counter" variant rapidly fails PractRand.
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief SQXOR 32-bit PRNG state.
 */
typedef struct {
    uint32_t w; ///< "Weyl sequence" counter state
} SqXor32State;


static inline uint64_t get_bits_raw(SqXor32State *obj)
{
    const uint32_t s = 0x9E3779B9;
    const uint32_t ww = obj->w += s; // "Weyl sequence" variant
    // Round 1
    uint64_t sq = ((uint64_t) ww) * ww; // |16bit|16bit||16bit|16bit||
    uint32_t x = (uint32_t) ((sq >> 32) ^ sq); // Middle squares (32 bits) + XORing
    // Round 2
    sq = (uint64_t) x * ww;
    x = (uint32_t) ((sq >> 32) ^ sq); // Middle squares (64 bits) + XORing
    // Return the result
    return x;
}


static void *create(const CallerAPI *intf)
{
    SqXor32State *obj = intf->malloc(sizeof(SqXor32State));
    obj->w = intf->get_seed32();
    return obj;
}


MAKE_UINT32_PRNG("SqXor32", NULL)
