/**
 * @file xoroshiro48w8ppp.c
 * @brief xoroshiro48w8+++ is a modification of xoroshiro++ with a 48-bit state
 * that operates 8-bit words and is suitable for 16-bit CPUs such as retro
 * platforms and microcontrollers. Its period is \f$ 2^{48} - 1 \f$.
 * @details The period of this PRNG is too small for any serious application,
 * so it is more a toy/hack for constrained conditions. 
 *
 * Recommended shifts triples that give the full period:
 * `[3 1 4]`, `[7 1 4]`, `[6 1 5]` (the `[3 1 4]` has slightly higher
 * statistical quality).
 *
 * - SmokeRand: passes `express`, `brief`, `default` and `full`.
 * - PractRand 0.96: fails at 8 TiB
 *
 * The xoroshiro++ PRNG family was suggested by D. Blackman and S. Vigna,
 * see the https://doi.org/10.48550/arXiv.1805.01407 reference. The shifts
 * for this 8-bit version were optimized by A.L. Voskov.
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
    uint8_t s[6];
} Xoroshiro48w8PPPState;


static inline uint8_t Xoroshiro48w8PPPState_get_bits(Xoroshiro48w8PPPState *obj)
{
    const uint8_t s0 = obj->s[0];
    const uint8_t s0x5 = (uint8_t) (s0 ^ obj->s[5]);
    const uint8_t ans = (uint8_t) (rotl8(s0 + obj->s[5], 1) + s0 + obj->s[3]);
    obj->s[0] = obj->s[1];
    obj->s[1] = obj->s[2];
    obj->s[2] = obj->s[3];
    obj->s[3] = obj->s[4];
    obj->s[4] = (uint8_t) (rotl8(s0, 3) ^ s0x5 ^ (s0x5 << 1));
    obj->s[5] = rotl8(s0x5, 4);
    return ans;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t x = 0;
    for (int i = 0; i < 4; i++) {
        x <<= 8;
        x |= Xoroshiro48w8PPPState_get_bits(state);
    }
    return x;
}


static void *create(const CallerAPI *intf)
{
    Xoroshiro48w8PPPState *obj = intf->malloc(sizeof(Xoroshiro48w8PPPState));
    const uint64_t seed = intf->get_seed64();
    for (int i = 0; i < 6; i++) {
        obj->s[i] = (uint8_t) (seed >> (8 * i));
    }
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[0] = 0x12;
        obj->s[1] = 0x34;
    }
    return obj;
}

MAKE_UINT32_PRNG("xoroshiro48w8+++", NULL)
