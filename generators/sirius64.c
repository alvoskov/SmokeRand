/**
 * @file sirius64.c
 * @brief Sirius64 generator is based on scrambling of "discrete Weyl sequence".
 * @details The scrambler is non-bijective that allows it to pass the
 * 64-bit collision test.
 *
 * References:
 *
 * - https://github.com/matteo65/Sirius64
 *
 * @copyright Sirius64 PRNG was developed by Matteo Zapparoli.
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
 * @brief Sirius64 PRNG state.
 */
typedef struct {
    uint64_t x;
} Sirius64State;

static inline uint64_t get_bits_raw(Sirius64State *obj)
{
    const uint64_t gamma = 0x9E3779B97F4A7C15U;
    uint64_t z = (obj->x += gamma);
    z = gamma * (z ^ (z >> 17));
    z = rotl64(z, 32);
    return gamma * (obj->x ^ z ^ (z >> 17));
}

static void *create(const CallerAPI *intf)
{
    Sirius64State *obj = intf->malloc(sizeof(Sirius64State));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("Sirius64", NULL)
