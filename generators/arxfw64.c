/**
 * @file arxfw64.c
 * @brief ARX-FW-64 experimental chaotic generator.
 * @details A simple and moderately fast chaotic generator. Its period
 * cannot be less than 2^64 due to a linear counter-based part.
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
 * @brief arxfw64 PRNG state.
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t w;
} Arxfw64State;

static inline uint64_t get_bits_raw(Arxfw64State *obj)
{
    uint64_t a = obj->a, b = obj->b;
    const uint64_t out = a ^ b;
    b += obj->w;
    a += ( rotl64(b, 13) ^ rotl64(b, 32) ^ b );
    b ^= ( rotl64(a, 57) + rotl64(a, 32) + a );
    obj->a = b;
    obj->b = a;
    obj->w++;
    return out;
}

static void *create(const CallerAPI *intf)
{
    Arxfw64State *obj = intf->malloc(sizeof(Arxfw64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->w = intf->get_seed64();
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("arxfw64", NULL)
