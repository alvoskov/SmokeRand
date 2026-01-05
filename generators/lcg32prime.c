/**
 * @file lcg32prime.c
 * @brief An implementation of classic 32-bit MCG with prime modulus.
 * @details The multiplier is taken from:
 *
 *  1. P. L'Ecuyer. Tables of linear congruential generators of different
 *  sizes and good lattice structure // Mathematics of Computation. 1999.
 *  V. 68. N. 225. P. 249-260
 *  http://dx.doi.org/10.1090/S0025-5718-99-00996-5
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg32State *obj)
{
    const uint64_t a = 1588635695UL;
    const uint64_t m = 4294967291UL; // 2^32 - 5
    const uint64_t c = 123U;
#if SIZE_MAX == UINT32_MAX
    // Implementation for 32-bit systems;
    // 64-bit mod may require runtime library funcions on such platforms
    // (e.g. 32-bit MinGW). So we use custom implementation.
    const uint64_t d = 5;
    const uint64_t ax = a * obj->x + c;
    const uint64_t lo = ax & 0xFFFFFFFF, hi = ax >> 32;
    uint64_t r = lo + d * hi;
    const int k = (int) (r >> 32) - 1;
    if (k > 0) {
        r -= ((uint64_t) k << 32) - (uint64_t) k * d;
    }
    if (r > m) {
        r -= m;
    }
    obj->x = (uint32_t) r;
#else
    // Implementation for 64-bit systems
    obj->x = (uint32_t) ( (a * obj->x + c) % m );
#endif
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed32();
    return obj;
}

/**
 * @brief Internal self-test.
 */
static int run_self_test(const CallerAPI *intf)
{
    Lcg32State obj = {.x = 1};
    uint64_t u, u_ref = 4055904884UL;
    for (long i = 0; i < 100000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llu; reference value: %llu\n", u, u_ref);
    return u == u_ref;
}



MAKE_UINT32_PRNG("LCG32Prime", run_self_test)
