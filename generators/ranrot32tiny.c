/**
 * @file ranrot32tiny.c
 * @brief A modified RANROT generator with guaranteed minimal period 2^32
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
    uint32_t m1;
    uint32_t m2;
    uint32_t m3;
    uint32_t w;
} RanRot32Tiny;


static uint64_t get_bits_raw(void *state)
{
    RanRot32Tiny *obj = state;
    obj->w += 0x9E3779B9;
    uint32_t u = rotl32(obj->m1, 11) + rotl32(obj->m3, 7);
    u = u + rotl32(obj->w ^ (obj->w >> 16), obj->m2 & 0x1F);
    obj->m3 = obj->m2;
    obj->m2 = obj->m1;
    obj->m1 = u;
    return u;
}

static void *create(const CallerAPI *intf)
{
    RanRot32Tiny *obj = intf->malloc(sizeof(RanRot32Tiny));
    seed64_to_2x32(intf, &obj->m1, &obj->m2);
    seed64_to_2x32(intf, &obj->m3, &obj->w);
    return obj;
}


MAKE_UINT32_PRNG("ranrot32tiny", NULL)
