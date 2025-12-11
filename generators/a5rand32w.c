/**
 * @file a5rand32w.c
 * @brief a5rand generator is a nonlinear chaotic pseudorandom number
 * generator suggested by Aleksey Vaneev. The algorithm description and
 * official test vectors can be found at https://github.com/avaneev/komihash
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Don't use this generator for statistical, scientific and
 * engineering computations!
 *
 * @copyright The a5rand algorithm was developed by Aleksey Vannev.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief a5rand PRNG state.
 */
typedef struct {
    uint32_t st1;
    uint32_t st2;
    uint32_t w;
} A5Rand32WeylState;


static inline uint64_t get_bits_raw(A5Rand32WeylState *obj)
{
    static const uint32_t inc1 = 0x55555555;
    obj->w += 0x9E3779B9;
    const uint64_t mul = (uint64_t)(obj->st1 + inc1) * (uint64_t) (obj->st2 + obj->w);
    obj->st1 = (uint32_t) (mul & 0xFFFFFFFF); // lower
    obj->st2 = (uint32_t) (mul >> 32); // higher
    return obj->st1 ^ obj->st2;
}


static void *create(const CallerAPI *intf)
{
    A5Rand32WeylState *obj = intf->malloc(sizeof(A5Rand32WeylState));
    obj->st1 = intf->get_seed32();
    obj->st2 = intf->get_seed32();
    obj->w   = intf->get_seed32();
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("a5rand32-Weyl", NULL)
