/**
 * @file rrmxmx.c
 * @brief A modified version of SplitMix that is resistant to bad gammas,
 * i.e. increments in the "discrete Weyl sequence".
 * @details It is the counter-based pseudorandom number generator suggested
 * by P.Evensen. Its mixing function is inspired by 
 *
 * References:
 *
 * 1. Pelle Evensen. On the mixing functions in "Fast Splittable Pseudorandom
 *    Number Generators", MurmurHash3 and David Stafford's improved variants
 *    on the MurmurHash3 finalizer.
 *    https://mostlymangling.blogspot.com/2018/07/on-mixing-functions-in-fast-splittable.html
 *
 * @copyright The rrmxmx algorithm was suggested by Pelle Evensen.
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
 * @brief RRMXMX PRNG state.
 */
typedef struct {
    uint64_t x;
} RrmxmxState;

#define GAMMA_GOLDEN 0x9E3779B97F4A7C15U

/**
 * @brief The RRMXMX implementation that uses the default gamma (fractional
 * part of golden ratio) from SplitMix.
 */
static inline uint64_t get_bits_raw(void *state)
{
    RrmxmxState *obj = state;
    static uint64_t const M = 0x9fb21c651e98df25;
    uint64_t v = obj->x += GAMMA_GOLDEN; // even obj->x++ is enough for BigCrush
    v ^= rotr64(v, 49) ^ rotr64(v, 24);
    v *= M;
    v ^= v >> 28;
    v *= M;
    return v ^ v >> 28;
}

static void *create(const CallerAPI *intf)
{
    RrmxmxState *obj = intf->malloc(sizeof(RrmxmxState));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

/**
 * @brief An internal self-test based on values from the original post
 * by P.Evensen.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref = 0x8fec24c21c6d66de;
    RrmxmxState obj;
    obj.x = 0xfedcba9876543210;
    obj.x -= GAMMA_GOLDEN;
    uint64_t u = get_bits_raw(&obj);
    intf->printf("Output: %llX; reference: %llX\n",
        (unsigned long long) u,
        (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("rrmxmx", run_self_test)
