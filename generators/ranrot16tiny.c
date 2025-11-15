/**
 * @file ranrot16tiny.c
 * @brief A modified RANROT generator with guaranteed minimal period 2^16
 * due to injection of the discrete Weyl sequence in its state. It is
 * a modification of RANROT PRNG made by A.L. Voskov.
 * pseudorandom number generator.
 * @details The RANROT generators were suggested by Agner Fog.
 *
 *  1. Agner Fog. Chaotic Random Number Generators with Random Cycle Lengths.
 *     2001. https://www.agner.org/random/theory/chaosran.pdf
 *  2. https://www.agner.org/random/discuss/read.php?i=138#138
 *  3. https://pracrand.sourceforge.net/
 *
 * @copyright 
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t m1;
    uint16_t m2;
    uint16_t m3;
    uint16_t w;
} RanRot16Tiny;


static uint16_t RanRot16Tiny_next(RanRot16Tiny *obj)
{
    obj->w += 0x9E37;
    uint16_t u = (uint16_t) (rotl16(obj->m1, 7) + rotl16(obj->m3, 3));
    u = (uint16_t) (u + rotl16(obj->w ^ (obj->w >> 8), obj->m2 & 0xF));
    obj->m3 = obj->m2;
    obj->m2 = obj->m1;
    obj->m1 = u;
    return u;
}


static uint64_t get_bits_raw(void *state)
{
    const uint32_t hi = RanRot16Tiny_next(state);
    const uint32_t lo = RanRot16Tiny_next(state);
    return (hi << 16) | lo;
}

static void *create(const CallerAPI *intf)
{
    RanRot16Tiny *obj = intf->malloc(sizeof(RanRot16Tiny));
    uint64_t seed = intf->get_seed64();
    obj->m1 = (uint16_t) seed;
    obj->m2 = (uint16_t) (seed >> 16);
    obj->m3 = (uint16_t) (seed >> 32);
    obj->w  = (uint16_t) (seed >> 48);
    return obj;
}


MAKE_UINT32_PRNG("ranrot16tiny", NULL)
