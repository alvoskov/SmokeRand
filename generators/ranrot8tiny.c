/**
 * @file ranrot8tiny.c
 * @brief A modified RANROT generator with guaranteed minimal period 2^8
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
    uint8_t m1;
    uint8_t m2;
    uint8_t m3;
    uint8_t w;
} RanRot8Tiny;


static uint8_t RanRot8Tiny_next(RanRot8Tiny *obj)
{
    obj->w += 151u;
    uint8_t u = (uint8_t) (rotl8(obj->m1, 5) + rotl8(obj->m3, 3));
    u = (uint8_t) (u + rotl8(obj->w ^ (obj->w >> 4), obj->m2 & 0x7));
    obj->m3 = obj->m2;
    obj->m2 = obj->m1;
    obj->m1 = u;
    return u;
}


static uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t u8[4];
        uint32_t u32;
    } out;
    out.u8[0] = RanRot8Tiny_next(state);
    out.u8[1] = RanRot8Tiny_next(state);
    out.u8[2] = RanRot8Tiny_next(state);
    out.u8[3] = RanRot8Tiny_next(state);
    return out.u32;
}

static void *create(const CallerAPI *intf)
{
    RanRot8Tiny *obj = intf->malloc(sizeof(RanRot8Tiny));
    uint64_t seed = intf->get_seed64();
    obj->m1 = (uint8_t) seed;
    obj->m2 = (uint8_t) (seed >> 8);
    obj->m3 = (uint8_t) (seed >> 16);
    obj->w  = (uint8_t) (seed >> 24);
    return obj;
}


MAKE_UINT32_PRNG("ranrot8tiny", NULL)
