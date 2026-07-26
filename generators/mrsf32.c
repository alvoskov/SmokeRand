/**
 * @file mrsf64.c
 * @brief mrsf32 is a very fast chaotic PRNG.
 * @details This generator is taken from PractRand 0.96 source code.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * @copyright mrsf32 algorithm is developed by Chris Doty-Humphrey,
 * the author of PractRand (https://sourceforge.net/projects/pracrand/).
 * 
 * Adaptation for SmokeRand:
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t a;
    uint32_t b;
} Mrsf32State;


static inline uint64_t get_bits_raw(Mrsf32State *obj)
{
    const uint32_t old = obj->a;
    obj->a = obj->b * 0xD3519D65u;
	obj->b = rotl32(obj->b, 13) ^ old;
	return old + obj->a;
}

static void *create(const CallerAPI *intf)
{
    Mrsf32State *obj = intf->malloc(sizeof(Mrsf32State));
    obj->a = intf->get_seed32();
    if (obj->a == 0) {
        obj->a = 0x12345678;
    }
    obj->b = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("Mrsf32", NULL)
