/**
 * @file mrsf64.c
 * @brief mrsf64 is a very fast chaotic PRNG.
 * @details This generator is taken from PractRand 0.96 source code.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * @copyright mrsf64 algorithm is developed by Chris Doty-Humphrey,
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
    uint64_t a;
    uint64_t b;
} Mrsf64State;


static inline uint64_t get_bits_raw(Mrsf64State *obj)
{
    const uint64_t old = obj->a;
    obj->a = obj->b * 0xAE3769b9D3519D65ull;
	obj->b = rotl64(obj->b, 23) ^ old;
	return old + obj->a;
}

static void *create(const CallerAPI *intf)
{
    Mrsf64State *obj = intf->malloc(sizeof(Mrsf64State));
    obj->a = 0x123456789ABCDEF;
    obj->b = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("Mrsf64", NULL)
