/**
 * @file ranval64.c
 * @brief A 64-bit chaotic generator based on the flea32x1 PRNG suggested by Bob Jenkins.
 * Also known as rkiss64 and jsf64.
 * @details A simple non-linear PRNG that passes all statistical tests
 * from SmokeRand, TestU01 and PractRand batteries. There were several
 * modifications of ranval, the implemented variant is from Bob Jenkins homepage.
 *
 * WARNING! THE MINIMAL PERIOD OF RANVAL IS UNKNOWN! Don't use it as a general
 * purpose pseudorandom number generator! Bad seeds are possible!
 *
 * References:
 *
 * 1. Bob Jenkins. The testing and design of small state noncryptographic
 *    pseudorandom number generators
 *    https://burtleburtle.net/bob/rand/talksmall.html
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Ranval64 PRNG state.
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
} Ranval64State;


static inline uint64_t get_bits_raw(void *state)
{
    Ranval64State *obj = state;
    const uint64_t e = obj->a - rotl64(obj->b,  7);
    obj->a = obj->b ^ rotl64(obj->c, 13);
    obj->b = obj->c + rotl64(obj->d, 37);
    obj->c = obj->d + e;
    obj->d = e + obj->a;
    return obj->d;
}

static void *create(const CallerAPI *intf)
{
    Ranval64State *obj = intf->malloc(sizeof(Ranval64State));
    uint64_t seed = intf->get_seed64();
    obj->a = 0xf1ea5eed;
    obj->b = obj->c = obj->d = seed;
    for (int i = 0; i < 20; i++) {
        (void) get_bits_raw(obj);
    }
    return (void *) obj;
}

MAKE_UINT64_PRNG("Ranval64", NULL)
