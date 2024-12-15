/**
 * @file lcg128_u32_full_shared.c
 * @brief Just 128-bit LCG with 128-bit multiplier and 32-bit output.
 * It is taken from:
 *
 * Steele G.L., Vigna S. Computationally easy, spectrally good multipliers
 * for congruential pseudorandom number generators // Softw Pract Exper. 2022
 * V. 52. N. 2. P. 443-458. https://doi.org/10.1002/spe.3030
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief A cross-compiler implementation of 128-bit LCG. Returns only upper
 * 32 bits.
 */
static inline uint64_t get_bits_raw(void *state)
{
//    return (Lcg128State_a128_iter(state, 0xdb36357734e34abb, 0x0050d0761fcdfc15, 1) >> 7) & 0xFFFFFFFF;
    Lcg128State_a128_iter(state, 0xdb36357734e34abb, 0x0050d0761fcdfc15, 1);
    Lcg128State *obj = state;
    return (obj->x >> 40) & 0xFFFFFFFF;
}


static void *create(const CallerAPI *intf)
{
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));
    Lcg128State_seed(obj, intf);
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
    uint64_t u, u_ref = 0x23fe67ff;// a50c941f;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("Lcg128_u32", run_self_test)
