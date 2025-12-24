/**
 * @file arxfw8ex3.c
 * @brief ARX-FX8-EX3 is a combined generator that consists of
 * chaotic part and linear LFSR and discrete Weyl sequence parts. Designed
 * for 8-bit processors.
 * @details LFSR part has a period of \f$2^{32} - 1\f$ (suggested by Edward
 * Rosten), chaotic part (designed by A.L.Voskov) is based on an invertible
 * mapping. Dicrete Weyl sequence has a period of 256. The main design goal
 * was to implement a PRNG with decent quality (passes SmokeRand ????,
 * TestU01 ????, PractRand 0.94 at least up ???) that is friendly
 * to 8-bit processors.
 *
 * WARNING! The minimal guaranteed period is only about \f$2^{40}\f$, the average
 * period is small and is only about 2^47, bad seeds are theoretically possible.
 * Usage of this generator for statistical, scientific and engineering
 * computations is strongly discouraged!
 *
 * References:
 *
 * 1. Edward Rosten. https://github.com/edrosten/8bit_rng
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief arxfw8ex3 PRNG state.
 */
typedef struct {
    uint8_t a; ///< Chaotic part
    uint8_t b; ///< Chaotic part
    uint8_t xs[4]; ///< LFSR part
    uint8_t w; ///< Discrete Weyl sequence part
} Arxfw8Ex3State;

static inline uint8_t get_bits8(Arxfw8Ex3State *obj)
{
    // LFSR part
    uint8_t *xs = obj->xs;    
    const uint8_t t = (uint8_t) (xs[0] ^ (xs[0] >> 1));
    xs[0] = xs[1];
    xs[1] = xs[2];
    xs[2] = xs[3];
    xs[3] = (uint8_t) (xs[2] ^ t ^ (xs[2] >> 3) ^ (t << 1));
    // Discrete Weyl sequence part
    obj->w = (uint8_t) (obj->w + 151U);
    // ARX-FW mixer part: it is simplified because it is driven
    // by LFSR
    uint8_t a = obj->a, b = obj->b;
    b = (uint8_t) (b + xs[3] + obj->w); // LFSR/Weyl injector
    a = (uint8_t) (a + (rotl8(b, 1) ^ rotl8(b, 4) ^ b));
    obj->a = b; obj->b = a;
    return obj->a ^ obj->b;
}

static inline uint64_t get_bits_raw(Arxfw8Ex3State *state)
{
    const uint32_t a = get_bits8(state);
    const uint32_t b = get_bits8(state);
    const uint32_t c = get_bits8(state);
    const uint32_t d = get_bits8(state);
    return a | (b << 8) | (c << 16) | (d << 24);
}


static void *create(const CallerAPI *intf)
{
    Arxfw8Ex3State *obj = intf->malloc(sizeof(Arxfw8Ex3State));
    uint64_t seed = intf->get_seed64();
    obj->a = (uint8_t) seed;
    obj->b = (uint8_t) (seed >> 8);
    obj->xs[0] = (uint8_t) (seed >> 16);
    obj->xs[1] = (uint8_t) (seed >> 24);
    obj->xs[2] = (uint8_t) (seed >> 32);
    obj->xs[3] = (uint8_t) (seed >> 40) | 0x1;
    obj->w     = (uint8_t) (seed >> 48);
        
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("arxfw8ex3", NULL)
