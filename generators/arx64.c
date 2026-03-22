/**
 * @file arx64.c
 * @brief arx64 (add, bit rotate, xor) pseudorandom number generator;
 * it is a modification of arx32 from PractRand 0.94.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * @copyright The arx32 algorithm was found in the sources of PractRand 0.94
 * that was developed by Chris Doty-Humphrey.
 *
 * 64-bit modification:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief arx64 (add, bit rotate, xor) generator state
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
} Arx64State;

static inline uint64_t get_bits_raw(Arx64State *obj)
{
    obj->a ^= rotl64(obj->b + obj->c, 13);
    obj->b ^= rotl64(obj->c + obj->a, 23);
    obj->c ^= rotl64(obj->a + obj->b, 31);
    return obj->a;
}


static void *create(const CallerAPI *intf)
{
    Arx64State *obj = intf->malloc(sizeof(Arx64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64() | 0x1;
    return obj;
}

MAKE_UINT64_PRNG("arx64", NULL)
