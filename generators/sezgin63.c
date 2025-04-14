/**
 * @file sezgin63.c
 * @brief Implementation of 63-bit LCG with prime modulus.
 * @details Systematically gives suspiciuos p-values at `default` battery
 * and fails the `full` battery. It fails the 2-dimensional birthday spacings
 * test for 32-bit values.
 *
 * In passes SmallCrush and Crush from TestU01, but systematically gives
 * suspect p-values at BigCrush test N13 (BirthdaySpacings, t = 2). It uses
 * 31-bit values instead of 32-bit ones and is slightly less sensitive than
 * the `bspace32_2d` and `bspace64_1d` tests from SmokeRand. It is interesting
 * that this PRNG passes PractRand 0.94.
 *
 * Although the authors consider this LCG [1] as a high-quality generator,
 * it shouldn't be uses as a general purpose PRNG: it systematically fails
 * the birthday spacings test.
 *
 * References:
 *
 * 1. Sezgin F., Sezgin T.M. Finding the best portable congruential random
 *    number generators // Computer Physics Communications. 2013. V. 184.
 *    N 8. P. 1889-1897. http://dx.doi.org/10.1016/j.cpc.2013.03.013
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 63-bit LCG state.
 */
typedef struct {
    int64_t x; ///< Must be signed!
} Lcg63State;


static inline uint64_t get_bits_raw(void *state)
{
    static const int64_t m = 9223372036854775783LL; // 2^63 - 25
    static const int64_t a = 3163036175LL; // See Line 4 in Table 1
    static const int64_t b = 2915986895LL;
    static const int64_t c = 2143849158LL;
    Lcg63State *obj = state;    
    obj->x = a * (obj->x % b) - c*(obj->x / b);
    if (obj->x < 0LL) {
        obj->x += m;
    }
    return obj->x >> 31;
}

static void *create(const CallerAPI *intf)
{
    Lcg63State *obj = intf->malloc(sizeof(Lcg63State));
    do {
        obj->x = intf->get_seed64() & ((1ull << 63) - 1);
    } while (obj->x == 0);
    return (void *) obj;
}

/**
 * @brief An internal self-test
 * @details The reference values were obtained from the next Python 3.x code:
 *
 *     a = 3163036175
 *     m = 9223372036854775783 # 2^63 - 25
 *     x = 1234567890
 *     for i in range(0,1000):
 *         x = (a * x) % m
 *         print(hex(x // 2**31))
 */
static int run_self_test(const CallerAPI *intf)
{
    Lcg63State obj = {.x = 1234567890};
    static const uint32_t x_ref = 0x3523699d;
    uint32_t x;
    for (int i = 0; i < 1000; i++) {
        x = get_bits_raw(&obj);
    }
    intf->printf("Output: %X; reference: %X\n",
        (unsigned int) x, (unsigned int) x_ref);
    return x == x_ref;
}

MAKE_UINT32_PRNG("Sezgin63", run_self_test)
