/**
 * @file lcg_hp48.c
 * @brief Decimal 48-bit pseudorandom number generator used in HP 48
 * programmable calculators.
 * @details It uses the next parameters set:
 *
 * \f[
 * x_{i+1} = 2851130928467 x_i \mod 10^{15}
 * \f]
 *
 * The next formula is used for fast modular reduction:
 *
 * \f[
 * (H 2^{64} + L) \mod m = \left(H(qm + r) + L\right) \mod m =
 * = (Hr + L) \mod m
 * \f]
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

#define MOD_10_POW_15 1000000000000000U

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    const uint64_t a = 2851130928467U;   // MCG multiplier
    const uint64_t r = 744073709551616U; // 2**64 % 10**15
    // a*x
    uint64_t prod_hi, prod_lo;
    prod_lo = unsigned_mul128(a, obj->x, &prod_hi);
    // a*x % m (modular reduction)
    do {
        prod_lo = unsigned_muladd128(r, prod_hi, prod_lo, &prod_hi);
    } while (prod_hi != 0);
    obj->x = prod_lo % MOD_10_POW_15;
    uint32_t out = (uint32_t) ((double) obj->x * 4.294967296e-06);
    return out;
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = (intf->get_seed64() % MOD_10_POW_15) | 1;
    if (obj->x % 5 == 0) { // 10**15 - 5 won't exceed the [0;10^15-1] interval
        obj->x += 2;       // in this branch
    }
    return obj;
}

/**
 * @brief An internal self-test based on a Python script output.
 * @details The used script:
 *
 *    x = 123456789
 *    for i in range(1_000_000):
 *        x = (2851130928467 * x) % 10**15
 *    print(x)
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint64_t x_ref = 731172143456789;
    Lcg64State obj = {.x = 123456789};
    for (int i = 0; i < 1000000; i++) {
        (void) get_bits_raw(&obj);
    }
    intf->printf("x=%llu x_ref=%llu\n",
        (unsigned long long) obj.x, (unsigned long long) x_ref);
    return (obj.x == x_ref) ? 1 : 0;
}

MAKE_UINT32_PRNG("LCG_HP48", run_self_test)

#undef MOD_10_POW_15
