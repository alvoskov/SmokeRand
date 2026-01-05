/**
 * @file ranrot64tiny.c
 * @brief A modified RANROT generator with guaranteed minimal period 2^64
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
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t m1;
    uint64_t m2;
    uint64_t m3;
    uint64_t w;
} RanRot64Tiny;


static uint64_t get_bits_raw(void *state)
{
    RanRot64Tiny *obj = state;
    obj->w += 0x9E3779B97F4A7C15;
    uint64_t u = rotl64(obj->m1, 25) + rotl64(obj->m3, 17);
    u = u + rotl64(obj->w ^ (obj->w >> 32), obj->m2 & 0x3F);
    obj->m3 = obj->m2;
    obj->m2 = obj->m1;
    obj->m1 = u;
    return u;
}

static void *create(const CallerAPI *intf)
{
    RanRot64Tiny *obj = intf->malloc(sizeof(RanRot64Tiny));
    obj->m1 = intf->get_seed64();
    obj->m2 = intf->get_seed64();
    obj->m3 = intf->get_seed64();
    obj->w  = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("ranrot64tiny", NULL)
