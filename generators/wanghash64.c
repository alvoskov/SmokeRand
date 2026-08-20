/**
 * @file wanghash64.c
 * @brief wanghash64 is a PRNG based on a wanghash64 hash function mixer
 * developed by Thomas Wang. It fails the `hamming_distr` test (its part that
 * uses XORed pairs).
 *
 * If it is processed through the `interleaved32` filter (i.e. 64-bit output
 * is processed as two 32-bit integers) - then it fails gap test, birthday
 * spacings test and collision tests in the `brief`/`default` batteries
 * of SmokeRand and Crush battery of TestU01.
 *
 * References:
 * 1. https://github.com/love2d/love/blob/main/src/modules/math/RandomGenerator.cpp
 * 2. https://web.archive.org/web/20110807030012/http://www.cris.com/%7ETtwang/tech/inthash.htm
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief WangHash64 PRNG state.
 */
typedef struct {
    uint64_t x;
} WangHash64State;


static inline uint64_t get_bits_raw(WangHash64State *obj)
{
    const uint64_t gamma = 0x9E3779B97F4A7C15U;
    uint64_t z = obj->x;
    z = (~z) + (z << 21);
    z ^= z >> 24;
    z += (z << 3) + (z << 8);
    z ^= z >> 14;
    z += (z << 2) + (z << 4);
    z ^= z >> 28;
    z += z << 31;
    obj->x += gamma;
    return z;
}


static void *create(const CallerAPI *intf)
{
    WangHash64State *obj = intf->malloc(sizeof(WangHash64State));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("WangHash64", NULL)
