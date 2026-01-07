/**
 * @file pcg32.c
 * @brief PCG32 PRNG implementation.
 * @details PCG (permuted congruental generators) is a family of pseudorandom
 * number generators invented by M.E. O'Neill. The PCG32 is a version with
 * 32-bit output and with 64-bit state. It passes all batteries from SmokeRand
 * test suite. It also passes SmallCrush, Crush and BigCrush batteries from
 * TestU01. However, it fails the TMFn test from PractRand 0.94 at 64 TiB.
 * 
 * @copyright The PCG32 algorithm is suggested by M.E. O'Neill
 * (https://pcg-random.org).
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef Lcg64State Pcg32State;

static inline uint64_t get_bits_raw(Pcg32State *obj)
{
    const uint32_t xorshifted = (uint32_t) ( ((obj->x >> 18) ^ obj->x) >> 27 );
    const int rot = (int) (obj->x >> 59);
    obj->x = obj->x * 6364136223846793005ull + 12345ull;
    return rotr32(xorshifted, rot);
}

static void *create(const CallerAPI *intf)
{
    Pcg32State *obj = intf->malloc(sizeof(Pcg32State));
    obj->x = intf->get_seed64();
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    Pcg32State obj = { .x = 0x123456789ABCDEF};
    static const uint32_t x_ref = 0x62435AA4;
    uint32_t x;
    for (int i = 0; i < 10000; i++) {
        x = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%lX; reference: 0x%lX\n",
        (unsigned long) x, (unsigned long) x_ref);
    return x == x_ref;

}

MAKE_UINT32_PRNG("PCG32", run_self_test)
