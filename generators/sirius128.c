/**
 * @file sirius128.c
 * @brief Sirius128 PRNG is based on scrambling of a 128-bit "discrete Weyl
 * sequence". Its architecture resembles SplitMix64 but uses 128->64 bit
 * non-bijective output function.
 * @details
 * 
 * @copyright Sirius128 algorithm was developed by Matteo Zapparoli.
 *
 * https://github.com/matteo65/Sirius128
 *
 * Reentrant plugin for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#include <stdint.h>
#include <stdio.h>

/**
 * @brief Sirius128 pseudorandom number generator state.
 */
typedef struct {
    uint64_t x; ///< The lower part of the "discrete Weyl sequence".
    uint64_t y; ///< The higher part of the "discrete Weyl sequence".
} Sirius128State;


static inline uint64_t get_bits_raw(Sirius128State *obj)
{
    const uint64_t w_hi = 0x9E3779B97F4A7C15ULL, w_lo = 0xF39CC0605CEDC835Ull;
    obj->y += w_hi + ((obj->x += w_lo) < w_lo);
    const uint64_t z = rotl64(w_hi * (obj->y ^ (obj->y >> 17)), 32);
    return w_hi * (obj->x ^ z ^ (z >> 17));
}


static void *create(const CallerAPI *intf)
{
    Sirius128State *obj = intf->malloc(sizeof(Sirius128State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    return obj;
}

/**
 * @brief An internal self-test based on the test value obtained
 * from the original implementation by Matteo Zapparoli.
 */
static int run_self_test(const CallerAPI *intf)
{
    Sirius128State obj = {.x = 12345, .y = 6789};
    const uint64_t u_ref = 0xC046990FFF928377;
    for (int i = 0; i < 10000; i++) {
        (void) get_bits_raw(&obj);
    }
    const uint64_t u = get_bits_raw(&obj);
    intf->printf("u=0x%llX u_ref=0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return (u == u_ref) ? 1 : 0;
}

MAKE_UINT64_PRNG("Sirius128", run_self_test)
