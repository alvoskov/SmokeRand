/**
 * @file komirand16w.c
 * @brief Komirand16-Weyl is a 16-bit modification of Komirand generator that
 * has an additional linear component - discrete Weyl sequence, this
 * modification provides a period at least 2^16 and average period of 2^32.
 * The modified algorithm was made by A.L. Voskov.
 * 
 * The original Komirand generator was suggested by Aleksey Vaneev. The
 * algorithm description and official test vectors can be found at
 * https://github.com/avaneev/komihash
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
 * @brief Komirand16-Weyl PRNG state.
 */
typedef struct {
    uint16_t st1;
    uint16_t st2;
    uint16_t w;
} Komirand16WeylState;

static inline uint16_t get_bits16(Komirand16WeylState *state)
{
    static const uint16_t inc = 0x9E37;
    uint16_t s1 = state->st1, s2 = state->st2;
    const uint32_t mul = (uint32_t) s1 * (uint32_t) s2;
    const uint16_t mul_lo = (uint16_t) (mul & 0xFFFF);
    const uint16_t mul_hi = (uint16_t) (mul >> 16);
    state->w = (uint16_t) (state->w + inc);
    s2 = (uint16_t) (s2 + mul_hi + state->w);
    s1 = (uint16_t) (mul_lo ^ s2);
    state->st1 = s1;
    state->st2 = s2;
    return s1;
}

static inline uint64_t get_bits_raw(Komirand16WeylState *state)
{
    const uint32_t a = get_bits16(state);
    const uint32_t b = get_bits16(state);
    return a | (b << 16);
}


static void *create(const CallerAPI *intf)
{
    Komirand16WeylState *obj = intf->malloc(sizeof(Komirand16WeylState));
    uint64_t seed = intf->get_seed64();
    obj->st1 = (uint16_t) seed;
    obj->st2 = (uint16_t) (seed >> 16);
    obj->w   = (uint16_t) (seed >> 32);
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("Komirand16Weyl", NULL)
