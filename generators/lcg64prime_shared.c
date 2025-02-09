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
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief A cross-compiler implementation of 64-bit LCG with prime modulus.
 */
static inline uint64_t get_bits_raw(void *state)
{
    Lcg64State *obj = state;
    const uint64_t a = 13891176665706064842ull;
    const uint64_t m = 18446744073709551557ull;  // 2^64 - 59
    const uint64_t d = 59;
    uint64_t hi, lo;
    lo = unsigned_mul128(a, obj->x, &hi);
#ifdef UINT128_ENABLED
    __uint128_t r = lo + d * (__uint128_t) hi;
    int k = (int) (r >> 64) - 1;
    if (k > 0) {
        r -= (((__uint128_t) k) << 64) - k * d;
    }
    if (r > m) {
        r -= m;
    }
    obj->x = r;
#else
    // r = lo + d*hi
    uint64_t r_lo, r_hi;
    r_lo = unsigned_mul128(d, hi, &r_hi);
    r_hi += _addcarry_u64(0, lo, r_lo, &r_lo);
    int k = ((int) (r_hi)) - 1;
    if (k > 0) {
        // r -= (k << 64) - k*d
        uint64_t kd_hi, kd_lo;
        kd_lo = unsigned_mul128((uint64_t)k, d, &kd_hi);
        r_hi = 1;
        unsigned char c = _addcarry_u64(0, r_lo, kd_lo, &r_lo);
        r_hi += kd_hi + c;
    }
    if (r_hi != 0 || r_lo > m) { // r > m
        r_lo -= m; // We don't care about r_hi: it will be thrown out
    }
    obj->x = r_lo;
#endif
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
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llu; reference value: %llu\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Lcg64prime", run_self_test)
