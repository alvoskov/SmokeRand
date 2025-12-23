/**
 * @file arxfw16ex2.c
 * @brief ARX-FW-16-EX2 experimental chaotic generator designed for 16-bit
 * processors and retrocomputing.
 * @details It was made as a toy generator to make a scaled down version of
 * arxfw64 PRNG suitable for 16-bit processors such as Intel 8086.
 * The "FW" means "Feystel-Weyl"
 
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
 * @brief arxfw16 PRNG state.
 */
typedef struct {
    uint16_t a; ///< Chaotic part
    uint16_t b; ///< Chaotic part
    uint16_t xs[2]; ///< LFSR (xorshift) part
    uint16_t w; ///< Discrete Weyl sequence part
} Arxfw16Ex2State;

static inline uint16_t get_bits16(Arxfw16Ex2State *obj)
{
    // LFSR part
    uint16_t *xs = obj->xs;
    const uint16_t t = (uint16_t) (xs[0] ^ (xs[0] << 1));
    xs[0] = xs[1];
    xs[1] = (uint16_t) ( (xs[1] ^ (xs[1] >> 7)) ^ (t ^ (t >> 1)) );
    // Weyl part
    obj->w = (uint16_t) (obj->w + 0x9E39);
    // ARX mixer part
    uint16_t a = obj->a, b = obj->b;
    b = (uint16_t) (b + xs[1] + obj->w);
    a = (uint16_t) (a + (rotl16(b, 3)  ^ rotl16(b, 8) ^ b));
    obj->a = b; obj->b = a;
    // Return value from ARX mixer
    return obj->a ^ obj->b;
}

static inline uint64_t get_bits_raw(Arxfw16Ex2State *state)
{
    const uint32_t a = get_bits16(state);
    const uint32_t b = get_bits16(state);
    return a | (b << 16);
}


static void *create(const CallerAPI *intf)
{
    Arxfw16Ex2State *obj = intf->malloc(sizeof(Arxfw16Ex2State));
    uint64_t seed = intf->get_seed64();
    obj->a = (uint16_t) seed;
    obj->b = (uint16_t) (seed >> 16);
    obj->xs[0] = (uint16_t) (seed >> 32);
    obj->xs[1] = (uint16_t) (seed >> 48) | 0x1;
    obj->w = 0;
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("arxfw16ex2", NULL)
