/**
 * @file lcg96_shared.c
 * @brief Just 96-bit LCG with \f$ m = 2^{128}\f$ and easy to memorize
 * multiplier 18000 69069 69069 69069 (suggested by A.L. Voskov)
 * @details It passes SmallCrush, Crush and BigCrush. However, its higher
 * 64 bits fail PractRand 0.94 at 128GiB sample. Usage of slightly better
 * (but hard to memorize) multiplier 0xfc0072fa0b15f4fd from 
 * https://doi.org/10.1002/spe.3030 doesn't improve PractRand 0.94 results.
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
    Lcg128State *obj = state;
    Lcg128State_a128_iter(obj, 0xdc879768, 0x60b11728995deb95, 1);
#ifdef UINT128_ENABLED
    obj->x = ((obj->x << 32) >> 32); // mod 2^96
    return (uint64_t)(obj->x >> 64);
#else
    // mod 2^96
    obj->x_high = ((obj->x_high << 32) >> 32);
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
    obj->x_high = 0;
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
    Lcg128State obj = {.x_low = 1234567890, .x_high = 0};
#endif
    uint64_t u, u_ref = 0xea5267e2;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("Lcg96", run_self_test)
