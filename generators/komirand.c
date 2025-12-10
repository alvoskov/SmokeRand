/**
 * @file komirand.c
 * @brief Komirand generator which is a wrapper around the komihash.h
 * library
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/cinterface.h"
#include "komihash.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Komirand PRNG state.
 */
typedef struct {
    uint64_t seed1;
    uint64_t seed2;
} komirand_state;

static inline uint64_t get_bits_raw(komirand_state *state)
{
	uint64_t seed1 = state->seed1;
	uint64_t seed2 = state->seed2;

	uint64_t ret = komirand(&seed1, &seed2);

	state->seed1 = seed1;
	state->seed2 = seed2;

	return ret;
}

static void *create(const CallerAPI *intf)
{
    komirand_state *obj = intf->malloc(sizeof(komirand_state));
    obj->seed1 = intf->get_seed64();
    obj->seed2 = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("Komirand", NULL)
