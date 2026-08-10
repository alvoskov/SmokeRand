/**
 * @file xoshiro512pp.c
 * @brief xoshiro512++ pseudorandom number generator.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna (vigna@acm.org).
 *
 * Reference:
 * 
 * 1. https://prng.di.unimi.it/xoshiro512plusplus.c
 *
 * @copyright
 * Refactoring with addition of internal self-tests:
 * 
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t s[8];
} Xoshiro512PPState;

static inline uint64_t get_bits_raw(Xoshiro512PPState *obj)
{
    uint64_t *s = obj->s;
    const uint64_t result = rotl64(s[0] + s[2], 17) + s[2];
    const uint64_t t = s[1] << 11;
    s[2] ^= s[0];
    s[5] ^= s[1];
    s[1] ^= s[2];
    s[7] ^= s[3];
    s[3] ^= s[4];
    s[4] ^= s[5];
    s[0] ^= s[6];
    s[6] ^= s[7];
    s[6] ^= t;
    s[7] = rotl64(s[7], 21);
    return result;
}

static void *create(const CallerAPI *intf)
{
    Xoshiro512PPState *obj = intf->malloc(sizeof(Xoshiro512PPState));
    seeds_to_array_u64(intf, obj->s, 8);
    if (obj->s[7] == 0) {
        obj->s[7] = 0x123456789ABCDEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("xoshiro512++", NULL)
