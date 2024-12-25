/**
 * @file ranval_shared.c
 * @brief Implementation of ranval PRNG suggested by Bob Jenkins.
 * @details A simple non-linear PRNG that passes almost all statistical tests
 * from SmokeRand, TestU01 and PractRand batteries. There were several
 * modifications of ranval, the implemented variant is from PractRand 0.94
 * by Chris Doty-Humphrey.
 *
 * WARNING! THE MINIMAL PERIOD OF RANVAL IS UNKNOWN! Don't use it as a general
 * purpose pseudorandom number generator!
 *
 * References:
 *
 * 1. Bob Jenkins. The testing and design of small state noncryptographic
 *    pseudorandom number generators
 *    https://burtleburtle.net/bob/rand/talksmall.html
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief ranval PRNG state.
 */
typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} RanvalState;


static inline uint32_t rotl32(uint32_t x, unsigned int r)
{
    return (x << r) | (x >> (32 - r));
}

static inline uint64_t get_bits_raw(void *state)
{
    RanvalState *obj = state;
    uint32_t e = obj->a - rotl32(obj->b, 23);
    obj->a = obj->b ^ rotl32(obj->c, 16);
    obj->b = obj->c + rotl32(obj->d, 11);
    obj->c = obj->d + e;
    obj->d = e + obj->a;
    return obj->d;
}

static void *create(const CallerAPI *intf)
{
    RanvalState *obj = intf->malloc(sizeof(RanvalState));
    obj->a = 0xf1ea5eed;
    obj->b = obj->c = obj->d = intf->get_seed32();
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return (void *) obj;
}

MAKE_UINT32_PRNG("ranval", NULL)
