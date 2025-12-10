/**
 * @file xoroshiro256pp.c
 * @brief xoroshiro256++ PRNG: https://prng.di.unimi.it/xoshiro256plusplus.c
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief PRNG state.
 */
typedef struct {
    uint64_t s[4];
} xoroshiro256_state;

static inline uint64_t rotl(const uint64_t x, int k)
{
	return (x << k) | (x >> (64 - k));
}

static inline uint64_t get_bits_raw(xoroshiro256_state *state)
{
	const uint64_t result = rotl(state->s[0] + state->s[3], 23) + state->s[0];

	const uint64_t t = state->s[1] << 17;

	state->s[2] ^= state->s[0];
	state->s[3] ^= state->s[1];
	state->s[1] ^= state->s[2];
	state->s[0] ^= state->s[3];

	state->s[2] ^= t;

	state->s[3] = rotl(state->s[3], 45);

	return result;
}

static void *create(const CallerAPI *intf)
{
    xoroshiro256_state *obj = intf->malloc(sizeof(xoroshiro256_state));

    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64();
    obj->s[2] = intf->get_seed64();
    obj->s[3] = intf->get_seed64();

    return (void *) obj;
}

MAKE_UINT64_PRNG("xoroshiro256pp", NULL)
