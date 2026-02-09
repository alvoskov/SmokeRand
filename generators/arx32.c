/**
 * @file arx32.c
 * @brief arx32 (add, bit rotate, xor) pseudorandom number generator
 * from PractRand 0.94. It has no lower boundary on its period.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * @copyright The arx32 algorithm was found in the sources of PractRand 0.94
 * that was developed by Chris Doty-Humphrey.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief arx32 (add, bit rotate, xor) generator state
 */
typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
} Arx32State;

static inline uint64_t get_bits_raw(Arx32State *obj)
{
    obj->a ^= rotl32(obj->b + obj->c, 7);
    obj->b ^= rotl32(obj->c + obj->a, 11);
    obj->c ^= rotl32(obj->a + obj->b, 15);
    return obj->a;
}


static void *create(const CallerAPI *intf)
{
    Arx32State *obj = intf->malloc(sizeof(Arx32State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->c = intf->get_seed32() | 0x1;
    return obj;
}

MAKE_UINT32_PRNG("arx32", NULL)
