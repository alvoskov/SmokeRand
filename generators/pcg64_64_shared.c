/**
 * @file pcg64_64_shared.c
 * @brief PCG64 PRNG implementation with RXS-M-XS64 function.
 * @details PCG (permuted congruental generators) is a family of pseudorandom
 * number generators invented by M.E. O'Neill. This version has 64-bit state
 * and 64-bit output and its period is \f$2^{64}\f$. It passes almost all
 * batteries from SmokeRand test suite except `birthday` battery that detects
 * absence of repeats in the output.
 *
 * PCG64 with RXS-M-XS64 output function also passes SmallCrush, Crush and
 * BigCrush batteries from TestU01.
 * 
 * @copyright The PCG algorithm family is suggested by M.E. O'Neill
 * (https://pcg-random.org).
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef Lcg64State Pcg64State;

static inline uint64_t get_bits_raw(void *state)
{
    Pcg64State *obj = state;
    uint64_t word = ((obj->x >> ((obj->x >> 59) + 5)) ^ obj->x) *
        12605985483714917081ull;
    obj->x = obj->x * 6364136223846793005ull + 1442695040888963407ull;
    return (word >> 43) ^ word;
}

static void *create(const CallerAPI *intf)
{
    Pcg64State *obj = intf->malloc(sizeof(Pcg64State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("PCG64", NULL)
