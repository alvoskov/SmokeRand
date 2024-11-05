/**
 * @file lcg128_full_shared.c
 * @brief Just 128-bit LCG with 128-bit multiplier. It is taken from:
 *
 * https://doi.org/10.1002/spe.3030
 *
 * TODO: MSVC ADAPTATION!
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 128-bit LCG state.
 * @details It has two versions: for compilers with 128-bit integers (GCC,Clang)
 * and for MSVC that has no such integers but has some compiler intrinsics
 * for 128-bit multiplication.
 */
typedef struct {
#ifdef UINT128_ENABLED
    unsigned __int128 x;
#else
    uint64_t x_low;
    uint64_t x_high;
#endif
} Lcg128State;

/**
 * @brief A cross-compiler implementation of 128-bit LCG.
 */
static inline uint64_t get_bits_raw(void *state)
{
    Lcg128State *obj = state;
    const unsigned __int128 a = ((unsigned __int128) 0xdb36357734e34abb) << 64 | 0x0050d0761fcdfc15;
#ifdef UINT128_ENABLED
    obj->x = a * obj->x + 1;
    return (uint64_t) (obj->x >> 64);
#else
    uint64_t mul0_high;
    obj->x_low = unsigned_mul128(a, obj->x_low, &mul0_high);
    obj->x_high = a * obj->x_high + mul0_high;
    obj->x_high += _addcarry_u64(0, obj->x_low, 1ull, &obj->x_low);
    return obj->x_high;
#endif
}


static void *create(const CallerAPI *intf)
{
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));
#ifdef UINT128_ENABLED
    obj->x = intf->get_seed64() | 0x1;
#else
    obj->x_low = intf->get_seed64() | 0x1;
#endif
    return (void *) obj;
}


/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
#ifdef UINT128_ENABLED
    Lcg128State obj = {.x = 1234567890};
#else
    Lcg128State obj = { .x_low = 1234567890, .x_high = 0 };
#endif
    uint64_t u, u_ref = 0x23fe67ffa50c941f;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Lcg128", run_self_test)
