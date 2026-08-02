/**
 * @file nasam.c
 * @brief NASAM generator based on scrambling of "discrete Weyl sequence".
 * It resembles SplitMix64 and a modified MurMur3 hash output function.
 * @details NASAM (Not Another Strange Acronym Mixer) was developed
 * by Pelle Evensen.
 *
 * References:
 *
 * - https://mostlymangling.blogspot.com/
 * - https://github.com/pellevensen
 *
 * @copyright The NASAM mixer/PRNG was developed by Pelle Evensen.
 *
 * Reentrant C99 implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief NASAM PRNG state.
 */
typedef struct {
    uint64_t x;
} NasamState;

static inline uint64_t get_bits_raw(NasamState *obj)
{
    uint64_t x = obj->x++;
    x ^= rotr64(x, 25) ^ rotr64(x, 47);
    x *= 0x9E6C63D0676A9A99U;
    x ^= (x >> 23) ^ (x >> 51);
    x *= 0x9E6D62D06F6A9A9BU;
    x ^= (x >> 23) ^ (x >> 51);
    return x;
}

static void *create(const CallerAPI *intf)
{
    NasamState *obj = intf->malloc(sizeof(NasamState));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("nasam", NULL)
