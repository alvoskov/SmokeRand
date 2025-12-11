/**
 * @file komirand32.c
 * @brief Komirand32 is a 32-bit modification of nonlinear chaotic pseudorandom number
 * generator suggested by Aleksey Vaneev. The algorithm description and
 * official test vectors can be found at https://github.com/avaneev/komihash
 *
 * This modification is a "toy generator" made only for demonstration and
 * research, fails the gap test in SmokeRand `brief` battery. Don't use it as
 * general purpose PRNG!
 *
 * @copyright The komirand algorithm was developed by Aleksey Vannev.
 *
 * The 32-bit version for testing:
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
 * @brief Komirand32 PRNG state.
 */
typedef struct {
    uint32_t st1;
    uint32_t st2;
} Komirand32State;

static inline uint64_t get_bits_raw(Komirand32State *state)
{
    static const uint32_t inc = 0xaaaaaaaa;
    uint32_t s1 = state->st1, s2 = state->st2;
    const uint64_t mul = (uint64_t) s1 * (uint64_t) s2;
    const uint32_t mul_lo = (uint32_t) (mul & 0xFFFFFFFF);
    const uint32_t mul_hi = (uint32_t) (mul >> 32);
    s2 += mul_hi + inc;
    s1 = mul_lo ^ s2;
    state->st1 = s1;
    state->st2 = s2;
    return s1;
}

static void *create(const CallerAPI *intf)
{
    Komirand32State *obj = intf->malloc(sizeof(Komirand32State));
    obj->st1 = intf->get_seed32();
    obj->st2 = obj->st1;
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("Komirand32", NULL)
