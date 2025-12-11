/**
 * @file komirand16.c
 * @brief Komirand16 is a 16-bit modification of nonlinear chaotic pseudorandom
 * number generator suggested by Aleksey Vaneev. The algorithm description and
 * official test vectors can be found at https://github.com/avaneev/komihash
 *
 * This modification is a "toy generator" made only for demonstration and
 * research. It fails a lot of tests!
 *
 * @copyright The komirand algorithm was developed by Aleksey Vannev.
 *
 * The 16-bit version for testing:
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
 * @brief Komirand16 PRNG state.
 */
typedef struct {
    uint16_t st1;
    uint16_t st2;
} Komirand16State;

static inline uint16_t get_bits16(Komirand16State *state)
{
    static const uint16_t inc = 0xaaaa;
    uint16_t s1 = state->st1, s2 = state->st2;
    const uint32_t mul = (uint32_t) s1 * (uint32_t) s2;
    const uint16_t mul_lo = (uint16_t) (mul & 0xFFFF);
    const uint16_t mul_hi = (uint16_t) (mul >> 16);
    s2 += (uint16_t) (mul_hi + inc);
    s1 = (uint16_t) (mul_lo ^ s2);
    state->st1 = s1;
    state->st2 = s2;
    return s1;
}

static inline uint64_t get_bits_raw(Komirand16State *state)
{
    const uint32_t a = get_bits16(state);
    const uint32_t b = get_bits16(state);
    return a | (b << 16);
}

static void *create(const CallerAPI *intf)
{
    Komirand16State *obj = intf->malloc(sizeof(Komirand16State));
    uint32_t seed = intf->get_seed32();
    obj->st1 = (uint16_t) seed;
    obj->st2 = obj->st1;
    //intf->printf("%X %X\n", (unsigned int) obj->st1, (unsigned int) obj->st2);
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("Komirand16", NULL)
