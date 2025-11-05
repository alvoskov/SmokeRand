/**
 * @file xoroshiro1024st.c
 * @brief xoroshiro1024* pseurorandom number generator.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna. This PRNG fails linearcomp test.
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
 * @copyright The xoroshiro1024* algorithm was suggested by D. Blackman
 * and S.Vigna.
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
 * @brief xoroshiro1024* PRNG state.
 */
typedef struct {
    int p;
    uint64_t s[16];
} Xoroshiro1024StarState;


static inline uint64_t get_bits_raw(void *state)
{
    Xoroshiro1024StarState *obj = state;
    const int q = obj->p;
    const uint64_t s0 = obj->s[obj->p = (obj->p + 1) & 15];
    uint64_t s15 = obj->s[q];
    const uint64_t result = s0 * 0x9e3779b97f4a7c13;
    s15 ^= s0;
    obj->s[q] = rotl64(s0, 25) ^ s15 ^ (s15 << 27);
    obj->s[obj->p] = rotl64(s15, 36);

    return result;
}

static void *create(const CallerAPI *intf)
{
    Xoroshiro1024StarState *obj = intf->malloc(sizeof(Xoroshiro1024StarState));
    obj->p = 0;
    for (size_t i = 0; i < 16; i++) {
        obj->s[i] = intf->get_seed64() | 0x1;
    }
    return (void *) obj;    
}


static int run_self_test(const CallerAPI *intf)
{
    Xoroshiro1024StarState *obj = intf->malloc(sizeof(Xoroshiro1024StarState));
    obj->p = 0;
    obj->s[0] = 0x123456789ABCDEF0;
    obj->s[1] = 0xDEADBEEFDEADBEEF;
    for (unsigned int i = 2; i < 16; i++) {
        obj->s[i] = 69069u * i;
    }
    static const uint64_t u_ref = 0x649D1DD3F9F676F5;
    uint64_t u;
    for (int i = 0; i < 1000000; i++) {
        u = get_bits_raw(obj);
    }
    intf->printf("Output: 0x%llX; reference value: 0x%llX\n",
        (unsigned long long) u,
        (unsigned long long) u_ref);
    intf->free(obj);
    return u == u_ref;
}


MAKE_UINT64_PRNG("xoroshiro1024*", run_self_test)
