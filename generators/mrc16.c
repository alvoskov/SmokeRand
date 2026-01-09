/**
 * @file mrc16.c
 * @brief mrc16 is a multiplication-base chaotic generator with the linear
 * "discrete Weyl sequence" part. It was developed by Chris Doty-Humphrey.
 * @details It is a 16-bit version of mrc64.
 *
 * WARNING! The minimal guaranteed period is only 2^16, the average
 * period is small is only about 2^47, bad seeds are theoretically possible.
 * Usage of this generator for statistical, scientific and engineering
 * computations is strongly discouraged!
 *
 * @copyright mrc16 algorithm is developed by Chris Doty-Humphrey,
 * the author of PractRand (https://sourceforge.net/projects/pracrand/).
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief mrc16 PRNG state.
 */
typedef struct {
    uint16_t a;
    uint16_t b;
    uint16_t ctr;
} Mrc16State;


static inline uint16_t get_bits16(Mrc16State *obj)
{
    const uint16_t old = (uint16_t) (obj->a * 0xA965U);
    obj->a = (uint16_t) (obj->b + obj->ctr++);
    obj->b = (uint16_t) (rotl16(obj->b, 10) ^ old);
    return (uint16_t) (old + obj->a);
}



static inline uint64_t get_bits_raw(Mrc16State *obj)
{
    const uint32_t a = get_bits16(obj);
    const uint32_t b = get_bits16(obj);
    return a | (b << 16);
}


static void *create(const CallerAPI *intf)
{
    Mrc16State *obj = intf->malloc(sizeof(Mrc16State));
    const uint64_t seed = intf->get_seed64();
    obj->a = (uint16_t) seed;
    obj->b = (uint16_t) (seed >> 16);
    obj->ctr = (uint16_t) (seed >> 32);
    for (int i = 0; i < 16; i++) {
        (void) get_bits16(obj);
    }
    return obj;
}


MAKE_UINT32_PRNG("Mrc16", NULL)
