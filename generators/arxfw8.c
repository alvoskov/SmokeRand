/**
 * @file arxfw8.c
 * @brief ARX-FW8 is a combined generator that consists of
 * chaotic part and 8-bit counter. Designed for 8-bit processors.
 * @details 8-bit coiunter ("discrete Weyl sequence") has a period of
 * \f$2^{8}\f$, chaotic part (designed by A.L.Voskov) is based on an invertible
 * mapping.
 *
 * WARNING! The minimal guaranteed period is only \f$2^{8}\f$, the average
 * period is small and is only about \f$2^{23}\f$, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * References:
 *
 * 1. Edward Rosten. https://github.com/edrosten/8bit_rng
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief arxfw8 PRNG state.
 */
typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t w;
} Arxfw8State;

static inline uint8_t get_bits8(Arxfw8State *obj)
{
    static const uint8_t inc = 0x9D;
    uint8_t a = obj->a, b = obj->b;
    b = (uint8_t) (b + obj->w);
    a = (uint8_t) (a + (rotl8(b, 1) ^ rotl8(b, 4) ^ b));
    b = (uint8_t) (b ^ (rotl8(a, 7) + rotl8(a, 4) + a));
    obj->a = b;
    obj->b = a;
    obj->w = (uint8_t) (obj->w + inc);
    return obj->a ^ obj->b;
}

static inline uint64_t get_bits_raw(Arxfw8State *state)
{
    const uint32_t a = get_bits8(state);
    const uint32_t b = get_bits8(state);
    const uint32_t c = get_bits8(state);
    const uint32_t d = get_bits8(state);
    return a | (b << 8) | (c << 16) | (d << 24);
}


static void *create(const CallerAPI *intf)
{
    Arxfw8State *obj = intf->malloc(sizeof(Arxfw8State));
    uint64_t seed = intf->get_seed64();
    obj->a = (uint8_t) seed;
    obj->b = (uint8_t) (seed >> 16);
    obj->w = (uint8_t) (seed >> 32);
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("arxfw8", NULL)
