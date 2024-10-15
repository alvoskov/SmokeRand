/**
 * @file lcg64prime_shared.c
 * @brief 64-bit LCG with prime modulus \f$ m = 2^{64} - 59 \f$.
 * @details It passes SmallCrush, Crush and BigCrush.
 *
 * References:
 *
 * 1. P. L'Ecuyer. Tables of linear congruential generators of different
 *    sizes and good lattice structure // Mathematics of Computation. 1999.
 *    V. 68. N. 225. P. 249-260
 *    http://dx.doi.org/10.1090/S0025-5718-99-00996-5
 * 2. https://en.wikipedia.org/wiki/Linear_congruential_generator
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 *
 * In scientific publications which used this software, a reference to it
 * would be appreciated.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 64-bit LCG state.
 */
typedef struct {
    uint64_t x;
} Lcg64State;

/**
 * @brief A cross-compiler implementation of 128-bit LCG.
 */
static uint64_t get_bits(void *state)
{
    Lcg64State *obj = state;
    const uint64_t a = 13891176665706064842ull;
    const uint64_t m = 18446744073709551557ull;  // 2^64 - 59
    const uint64_t d = 59;
    __uint128_t prod = (__uint128_t) a * obj->x;
    uint64_t hi = prod >> 64, lo = prod;
    __uint128_t r = lo + d * (__uint128_t) hi;
    int k = (int) (r >> 64) - 1;
    if (k > 0) {
        r -= (((__uint128_t) k) << 64) - k * d;
    }
    if (r > m) {
        r -= m;
    }
    obj->x = r;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64() | 0x1;
    return (void *) obj;
}


/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
    Lcg64State obj = {.x = 1};
    uint64_t u, u_ref = 3072923337735042611ull;
    for (size_t i = 0; i < 100000; i++) {
        u = get_bits(&obj);
    }
    intf->printf("Result: %llu; reference value: %llu\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Lcg64prime", run_self_test)
