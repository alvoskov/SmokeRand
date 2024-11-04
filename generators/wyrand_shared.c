/**
 * @file wyrand_shared.c
 * @brief wyrand pseudorandom number generator. Passes BigCrush and PractRand
 * batteries of statistical tests. Requires 128-bit integers.
 * @details References:
 * - Wang Yi. wyhash project, public domain (Unlicense).
 *   https://github.com/wangyi-fudan/wyhash/blob/master/wyhash.h
 * - testingRNG, wyrand.h file by D. Lemire (Apache 2.0 license)
 *   https://github.com/lemire/testingRNG/blob/master/source/wyrand.h
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} WyRandState;

uint64_t get_bits_raw(void *state)
{
    uint64_t hi, lo;
    WyRandState *obj = state;
    obj->x += 0xa0761d6478bd642f;
    lo = unsigned_mul128(obj->x, obj->x ^ 0xe7037ed1a0b428db, &hi);
    return lo ^ hi;
}

static void *create(const CallerAPI *intf)
{
    WyRandState *obj = intf->malloc(sizeof(WyRandState));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("WyRand", NULL)
