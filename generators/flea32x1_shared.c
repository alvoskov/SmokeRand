/**
 * @file flea32x1_shared.c
 * @brief Implementation of flea32x1 PRNG suggested by Bob Jenkins.
 * @details A simple non-linear PRNG that passes almost all statistical tests
 * except `mod3`. There were several modifications of flea, the implemented
 * variant is from PractRand 0.94 by Chris Doty-Humphrey.
 *
 * WARNING! THE MINIMAL PERIOD OF FLEA32x1 IS UNKNOWN! It was added mainly for
 * testing the `mod3` test and shouldn't be used in practice!
 *
 * References:
 *
 * 1. Bob Jenkins. The testing and design of small state noncryptographic
 *    pseudorandom number generators
 *    https://burtleburtle.net/bob/rand/talksmall.html
 * 2. https://pracrand.sourceforge.net/
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
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


static inline uint64_t get_bits_raw(void *state)
{
    enum { SHIFT1 = 15, SHIFT2 = 27 };
    Flea32x1State *obj = state;
    uint32_t e = obj->a;
    obj->a = (obj->b << SHIFT1) | (obj->b >> (32 - SHIFT1));
    obj->b = obj->c + ((obj->d << SHIFT2) | (obj->d >> (32 - SHIFT2)));
    obj->c = obj->d + obj->a;
    obj->d = e + obj->c;
    return obj->c;
}

static void *create(const CallerAPI *intf)
{
    Flea32x1State *obj = intf->malloc(sizeof(Flea32x1State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->c = intf->get_seed32();
    obj->d = intf->get_seed32();
    return (void *) obj;
}

MAKE_UINT32_PRNG("flea32x1", NULL)

