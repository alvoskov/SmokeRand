/**
 * @file lcg128.c
 * @brief Just 128-bit LCG with \f$ m = 2^{128}\f$ and easy to memorize
 * multiplier 18000 69069 69069 69069 (suggested by A.L. Voskov)
 * @details It passes SmallCrush, Crush and BigCrush. However, its higher
 * 64 bits fail PractRand 0.94 at 128GiB sample. Usage of slightly better
 * (but hard to memorize) multiplier 0xfc0072fa0b15f4fd from 
 * https://doi.org/10.1002/spe.3030 doesn't improve PractRand 0.94 results.
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
 * @brief A cross-compiler implementation of 128-bit LCG.
 */
static inline uint64_t get_bits_raw(void *state)
{
    const uint64_t a = 18000690696906969069ull;
    return Lcg128State_a64_iter(state, a, 1ull);
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
    Lcg128State_init(&obj, 0, 1234567890);
    uint64_t u, u_ref = 0x8E878929D96521D7;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Lcg128", run_self_test)
