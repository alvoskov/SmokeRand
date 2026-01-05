/**
 * @file flea32x1.c
 * @brief Implementation of flea32x1 PRNG suggested by Bob Jenkins.
 * @details A simple non-linear PRNG that passes almost all statistical tests
 * except `mod3`. There were several modifications of flea, the implemented
 * variant is from PractRand 0.94 by Chris Doty-Humphrey.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * The generator was added mainly for testing the `mod3` test: it is the
 * only test that it fails.
 *
 * References:
 *
 * 1. Bob Jenkins. The testing and design of small state noncryptographic
 *    pseudorandom number generators
 *    https://burtleburtle.net/bob/rand/talksmall.html
 * 2. https://pracrand.sourceforge.net/
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief flea32x1 PRNG state.
 */
typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} Flea32x1State;


static inline uint64_t get_bits_raw(Flea32x1State *obj)
{
    const uint32_t e = obj->a;
    obj->a = rotl32(obj->b, 15);
    obj->b = obj->c + rotl32(obj->d, 27);
    obj->c = obj->d + obj->a;
    obj->d = e + obj->c;
    return obj->c;
}

static void *create(const CallerAPI *intf)
{
    Flea32x1State *obj = intf->malloc(sizeof(Flea32x1State));
    seed64_to_2x32(intf, &obj->a, &obj->b);
    seed64_to_2x32(intf, &obj->c, &obj->d);
    return obj;
}

MAKE_UINT32_PRNG("flea32x1", NULL)

