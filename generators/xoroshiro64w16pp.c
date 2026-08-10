/**
 * @file xoroshiro64w16pp.c
 * @brief xoroshiro64w16++ is a modification of xoroshiro++ with a 64-bit state
 * that operates 16-bit words and is suitable for 16-bit CPUs such as retro
 * platforms and microcontrollers. Its period is \f$ 2^{64} - 1 \f$.
 * @details The parameters (shifts) were optimized by A.L. Voskov, the
 * recommended triple is [10,1,11] that was tested in SmokeRand and TestU01
 * without a ++ scrambler:
 *
 * - SmokeRand `full` battery: fails matrixrank/linearcomp and
 *   hammming_distr/hamming_ot tests but not such tests as birthday spacings,
 *   gap test etc.
 * - TestU01 Crush: fails matrixrank/linearcomp, also `RandomWalk1 H (L = 90)`
 *   and `WeightDistrib, r = 0`.
 *
 * The [10 1 7], [1 2 10], [3 4 8], [10 4 5] triples also provide a full period
 * but fail birthday spacings / collision test in SmokeRand batteries and
 * probably shouldn't be used.
 *
 * Scrambled version:
 *
 * - Passes `express`, `brief`, `default` and `full` SmokeRand batteries.
 * - Passes SmallCrush, Crush
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
    uint16_t s[4];
} Xoroshiro64w16PPState;


static inline uint16_t Xoroshiro64w16PPState_get_bits(Xoroshiro64w16PPState *obj)
{
    const uint16_t s0 = obj->s[0];
    const uint16_t res = (uint16_t) (rotl16((uint16_t) (s0 + obj->s[3]), 5) + s0);
    const uint16_t s0x3 = (uint16_t) (s0 ^ obj->s[3]);
    obj->s[0] = obj->s[1];
    obj->s[1] = obj->s[2];
    obj->s[2] = (uint16_t) (rotl16(s0, 10) ^ s0x3 ^ (s0x3 << 1));
    obj->s[3] = rotl16(s0x3, 11);
    return res;
}

static inline uint64_t get_bits_raw(void *state)
{
    const uint32_t hi = Xoroshiro64w16PPState_get_bits(state);
    const uint32_t lo = Xoroshiro64w16PPState_get_bits(state);
    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Xoroshiro64w16PPState *obj = intf->malloc(sizeof(Xoroshiro64w16PPState));
    const uint64_t seed = intf->get_seed64();
    obj->s[0] = (uint16_t) (seed >> 48);
    obj->s[1] = (uint16_t) (seed >> 32);
    obj->s[2] = (uint16_t) (seed >> 16);
    obj->s[3] = (uint16_t) seed;
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[0] = 0xDEAD;
        obj->s[1] = 0xBEEF;
    }
    return obj;
}

MAKE_UINT32_PRNG("xoroshiro64w16++", NULL)
