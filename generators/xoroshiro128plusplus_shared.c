/**
 * @file xoroshiro128plusplus_shared.c
 * @brief xoroshiro128++ pseurorandom number generator.
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
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

/**
 * @brief xoroshiro128++ PRNG state. Mustn't be initialized as (0, 0).
 */
typedef struct {
    uint64_t s[2];
} Xoroshiro128PlusPlusState;


static uint64_t get_bits(void *state)
{
    Xoroshiro128PlusPlusState *obj = state;
	const uint64_t s0 = obj->s[0];
	uint64_t s1 = obj->s[1];
	const uint64_t result = rotl(s0 + s1, 17) + s0;
	s1 ^= s0;
	obj->s[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
	obj->s[1] = rotl(s1, 28); // c
    return result;
}

static void *create(const CallerAPI *intf)
{
    Xoroshiro128PlusPlusState *obj = intf->malloc(sizeof(Xoroshiro128PlusPlusState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64() | 0x1;
    return (void *) obj;    
}

MAKE_UINT64_PRNG("xoroshiro128++", NULL)
