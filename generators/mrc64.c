/**
 * @file mrc64.c
 * @brief mrc64 is a multiplication-base chaotic generator with the linear
 * "discrete Weyl sequence" part. It was developed by Chris Doty-Humphrey.
 * @details It is a very fast algorithm that passes statistical tests well.
 * It is designed for 64-bit architectures.
 *
 * The minimal guaranteed period is 2^64, the average period is about 2^191.
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
 * @brief mrc64 PRNG state.
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t ctr;
} Mrc64State;


static inline uint64_t get_bits_raw(Mrc64State *obj)
{
    const uint64_t old = obj->a * 0x9e3779b97f4a7c15U;
    obj->a = obj->b + obj->ctr++;
    obj->b = rotl64(obj->b, 21) ^ old;
    return old + obj->a;
}


static void *create(const CallerAPI *intf)
{
    Mrc64State *obj = intf->malloc(sizeof(Mrc64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->ctr = intf->get_seed64();
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT64_PRNG("Mrc64", NULL)
