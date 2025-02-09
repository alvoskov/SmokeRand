/**
 * @file pcg32_shared.c
 * @brief PCG32 PRNG implementation.
 * @details PCG (permuted congruental generators) is a family of pseudorandom
 * number generators invented by M.E. O'Neill. The PCG32 is a version with
 * 32-bit output and with 64-bit state. It passes all batteries from SmokeRand
 * test suite. It also passes SmallCrush, Crush and BigCrush batteries from
 * TestU01.
 * 
 * @copyright The PCG32 algorithm is suggested by M.E. O'Neill
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

typedef Lcg64State Pcg32State;

static inline uint64_t get_bits_raw(void *state)
{
    Pcg32State *obj = state;
    uint32_t xorshifted = (uint32_t) ( ((obj->x >> 18) ^ obj->x) >> 27 );
    uint32_t rot = obj->x >> 59;
    obj->x = obj->x * 6364136223846793005ull + 12345;
    return (xorshifted << (32 - rot)) | (xorshifted >> rot);
}

static void *create(const CallerAPI *intf)
{
    Pcg32State *obj = intf->malloc(sizeof(Pcg32State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT32_PRNG("PCG32", NULL)
