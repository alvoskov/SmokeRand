/**
 * @file lcg128_full.c
 * @brief Just 128-bit LCG with 128-bit multiplier. It is taken from:
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
 * @brief A cross-compiler implementation of 128-bit LCG.
 */
static inline uint64_t get_bits_raw(void *state)
{
    return Lcg128State_a128_iter(state, 0xdb36357734e34abb, 0x0050d0761fcdfc15, 1);
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
    Lcg128State obj;
    Lcg128State_init(&obj, 1234567890, 0);
    uint64_t u, u_ref = 0x23fe67ffa50c941f;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Lcg128", run_self_test)
