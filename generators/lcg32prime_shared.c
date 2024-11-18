/**
 * @file lcg69069_shared.c
 * @brief An implementation of classic 32-bit MCG with prime modulus.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 32-bit LCG state.
 */
typedef struct {
    uint32_t x;
} Lcg32State;


static inline uint64_t get_bits_raw(void *state)
{
    Lcg32State *obj = state;
    const uint64_t a = 1588635695ul;
    const uint64_t m = 4294967291ul; // 2^32 - 5
//    const uint64_t d = 5;
    obj->x = (a * obj->x + 123) % m;
/*
    uint64_t ax = a * obj->x + 123;
    uint64_t lo = ax & 0xFFFFFFFF, hi = ax >> 32;
    uint64_t r = lo + d * hi;
    int k = (int) (r >> 32) - 1;
    if (k > 0) {
        r -= (((uint64_t) k) << 32) - k * d;
    }
    if (r > m) {
        r -= m;
    }
    obj->x = r;
*/
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = (intf->get_seed64() >> 32) | 0x1;
    return (void *) obj;
}

/**
 * @brief Internal self-test.
 */
static int run_self_test(const CallerAPI *intf)
{
    Lcg32State obj = {.x = 1};
    uint64_t u, u_ref = 4055904884ul;
    for (size_t i = 0; i < 100000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llu; reference value: %llu\n", u, u_ref);
    return u == u_ref;
}



MAKE_UINT32_PRNG("LCG32Prime", run_self_test)
