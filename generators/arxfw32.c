/**
 * @file arxfw32.c
 * @brief ARX-FW-32 experimental chaotic generator.
 * @details A simple and moderately fast chaotic generator. Its period
 * cannot be less than 2^32 due to a linear counter-based part.
 * The "FW" means "Feystel-Weyl"
 *
 * WARNING! MINIMAL PERIOD IS 2^{32}! IT IS NOT ENOUGH FOR RELIABLE PRACTICAL
 * USAGE! Probability to achieve it is very low but exactly unknown.
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
 * @brief arxfw64 PRNG state.
 */
typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t w;
} Arxfw32State;

static inline uint64_t get_bits_raw(Arxfw32State *obj)
{
    uint32_t a = obj->a, b = obj->b;
    const uint32_t out = a ^ b;
    b += obj->w;
    a += ( rotl32(b, 7) ^ rotl32(b, 16) ^ b );
    b ^= ( rotl32(a, 13) + rotl32(a, 16) + a );
    obj->a = b;
    obj->b = a;
    obj->w++;
    return out;
}

static void *create(const CallerAPI *intf)
{
    Arxfw32State *obj = intf->malloc(sizeof(Arxfw32State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->w = intf->get_seed32();
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("arxfw32", NULL)
