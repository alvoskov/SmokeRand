/**
 * @file mrc32.c
 * @brief mrc32 is a multiplication-base chaotic generator with the linear
 * "discrete Weyl sequence" part. It was developed by Chris Doty-Humphrey.
 * @details It is a 32-bit version of mrc64.
 *
 * WARNING! The minimal guaranteed period is only 2^32, the average
 * period is about 2^95, bad seeds are theoretically possible.
 * Usage of this generator for statistical, scientific and engineering
 * computations is not recommended!
 *
 * @copyright mrc32 algorithm is developed by Chris Doty-Humphrey,
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
 * @brief mrc32 PRNG state.
 */
typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t ctr;
} Mrc32State;


static inline uint64_t get_bits_raw(Mrc32State *obj)
{
    const uint32_t old = obj->a * 0x7f4a7c15U;
    obj->a = obj->b + obj->ctr++;
    obj->b = rotl32(obj->b, 19) ^ old;
    return old + obj->a;
}


static void *create(const CallerAPI *intf)
{
    Mrc32State *obj = intf->malloc(sizeof(Mrc32State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->ctr = intf->get_seed32();
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT32_PRNG("Mrc32", NULL)



