/**
 * @file xoroshiro48w16pp.c
 * @brief xoroshiro48w16++ is a modification of xoroshiro++ with a 48-bit state
 * that operates 16-bit words and is suitable for 16-bit CPUs such as retro
 * platforms and microcontrollers. Its period is \f$ 2^{48} - 1 \f$.
 * @details The period of this PRNG is too small for any serious application,
 * so it is more a toy/hack for constrained conditions. However, it is still
 * fairly robust and passes SmokeRand `full` and TestU01 BigCrush batteries
 * but fails PractRand 0.96 at 1 TiB sample.
 *
 * The recommended shifts are `[7 6 2]`.
 *
 * Some intermediate results for not scrambled version:
 *
 * - `[3 6 1]`: brief 8/9 (including 5 collover/bspace)
 * - `[7 6 2]`: brief 6/1 (including 2/3 collover/bspace)
 * - `[9 5 13]`: brief 6/8 (includng 3/4 collover/bspace)
 * - `[15 5 3]`: brief 7/8 (include 3/4 collover/bspace)
 * - `[15 7 4]`: brief 6/9 (include 3/5 collover/bspace)
 *
 * The xoroshiro++ PRNG family was suggested by D. Blackman and S. Vigna,
 * see the https://doi.org/10.48550/arXiv.1805.01407 reference. The shifts
 * for this 16-bit versions were optimized by A.L. Voskov.
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t s[3];
} Xoroshiro48w16PPState;


static inline uint16_t Xoroshiro48w16PPState_get_bits(Xoroshiro48w16PPState *obj)
{
    const uint16_t s0 = obj->s[0];
    const uint16_t res = (uint16_t) (rotl16((uint16_t) (s0 + obj->s[2]), 5) + s0);
    const uint16_t s0x2 = (uint16_t) (s0 ^ obj->s[2]);
    obj->s[0] = obj->s[1];
    obj->s[1] = (uint16_t) (rotl16(s0, 7) ^ s0x2 ^ (s0x2 << 6));
    obj->s[2] = rotl16(s0x2, 2);
    return res;
}

static inline uint64_t get_bits_raw(void *state)
{
    const uint32_t hi = Xoroshiro48w16PPState_get_bits(state);
    const uint32_t lo = Xoroshiro48w16PPState_get_bits(state);
    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Xoroshiro48w16PPState *obj = intf->malloc(sizeof(Xoroshiro48w16PPState));
    const uint64_t seed = intf->get_seed64();
    obj->s[0] = (uint16_t) (seed >> 32);
    obj->s[1] = (uint16_t) (seed >> 16);
    obj->s[2] = (uint16_t) seed;
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[0] = 0xDEAD;
        obj->s[1] = 0xBEEF;
    }
    return obj;
}

MAKE_UINT32_PRNG("xoroshiro48w16++", NULL)
