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

typedef struct {
    uint64_t x;
    uint64_t y;
} Pcg32StreamsState;

static inline uint32_t pcg32_iter(uint64_t *x, const uint64_t inc)
{
    uint32_t xorshifted = (uint32_t) ( ((*x >> 18) ^ *x) >> 27 );
    uint32_t rot = *x >> 59;
    *x = *x * 6364136223846793005ull + inc;
    return (xorshifted << (32 - rot)) | (xorshifted >> rot);
}

static inline uint64_t get_bits_raw(void *state)
{
    Pcg32StreamsState *obj = state;
    uint64_t hi = pcg32_iter(&obj->x, 1);
    uint64_t lo = pcg32_iter(&obj->y, 3);
    return (hi << 32) | lo;
}

static void *create(const CallerAPI *intf)
{
    Pcg32StreamsState *obj = intf->malloc(sizeof(Pcg32StreamsState));
    obj->x = intf->get_seed32();
    obj->y = obj->x;
    return (void *) obj;
}

MAKE_UINT64_PRNG("PCG64", NULL)
