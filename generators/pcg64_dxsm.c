/**
 * @file pcg64_dxsm.c
 * @brief PCG64-DXSM (Double xor shift multiply) pseudorandom number generator.
 * @details This generator was suggested by Melissa O'Neill in 2019.
 *
 * References:
 *
 * https://github.com/numpy/numpy/issues/13635#issuecomment-506088698
 * @copyright The PCG-DXSM family of algorithms was suggested by M.O'Neill.
 *
 * Reentrant version for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg128State *obj)
{
    const uint64_t a = 0xda942042e4dd58b5ULL;
    // Just ordinary 128-bit LCG
    (void) Lcg128State_a64_iter(obj, a, 1);
    // Output DXSM (double xor, shift, multiply) function
    uint64_t high = obj->x_high;
    high ^= high >> 32;
    high *= a;
    high ^= high >> 48;
    return high * (obj->x_low | 0x1);
}


static void *create(const CallerAPI *intf)
{
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));
    Lcg128State_seed(obj, intf);
    return obj;
}

/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
    Lcg128State obj = {.x_low = 1234567890, .x_high = 0};
    uint64_t u, u_ref = 0xF833FBF625E74DAF;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("PCG64-DXSM", run_self_test)
