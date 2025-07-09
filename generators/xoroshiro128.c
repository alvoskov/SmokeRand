/**
 * @file xoroshiro128.c
 * @brief xoroshiro128 pseurorandom number generator.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna (vigna@acm.org). This PRNG fails matrixrank and linearcomp tests.
 *
 * References:
 * 1. D. Blackman, S. Vigna. Scrambled Linear Pseudorandom Number Generators
 *    // ACM Transactions on Mathematical Software (TOMS). 2021. V. 47. N 4.
 *    Article No.: 36, P.1-32. https://doi.org/10.1145/3460772
 * 2. D. Lemire, M. E. O'Neill. Xorshift1024*, xorshift1024+, xorshift128+
 *    and xoroshiro128+ fail statistical tests for linearity // Journal of
 *    Computational and Applied Mathematics. 2019. V.350. P.139-142.
 *    https://doi.org/10.1016/j.cam.2018.10.019.
 * 3. xoshiro / xoroshiro generators and the PRNG shootout
 *    https://prng.di.unimi.it/
 * @copyright The xoroshiro128+ algorithm was suggested by D. Blackman
 * and S. Vigna.
 *
 * Adaptation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief xoroshiro128 PRNG state. Mustn't be initialized as (0, 0).
 */
typedef struct {
    uint64_t s[2];
} Xoroshiro128State;


static inline uint64_t get_bits_raw(void *state)
{
    Xoroshiro128State *obj = state;
    const uint64_t s0 = obj->s[0];
    uint64_t s1 = obj->s[1];
    s1 ^= s0;
    obj->s[0] = rotl64(s0, 24) ^ s1 ^ (s1 << 16); // a, b
    obj->s[1] = rotl64(s1, 37); // c
    return s0;
}


static void *create(const CallerAPI *intf)
{
    Xoroshiro128State *obj = intf->malloc(sizeof(Xoroshiro128State));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64() | 0x1;
    return (void *) obj;    
}

MAKE_UINT64_PRNG("xoroshiro128", NULL)
