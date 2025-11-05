/**
 * @file xoroshiro64stst.c
 * @brief xoroshiro64** is a LFSR generator with a simple output scrambler.
 * @details The algorithm is developed by David Blackman and Sebastiano Vigna
 * (vigna@acm.org) in 2016. Its lower bits have no problems with low linear
 * complexity. However, the generator still fails 64-bit birthday paradox
 * test.
 *
 * References:
 *
 * 1. https://prng.di.unimi.it/xoroshiro64star.c
 * 2. https://prng.di.unimi.it/xoroshiro64starstar.c
 *
 * @copyright xoroshiro64** algorithms were suggested by D.Blackman
 * and S.Vigna. Reentrant version for SmokeRand:
 * 
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief xoroshiro64** generator state.
 */
typedef struct {
    uint32_t s[2];    
} Xoroshiro64StStState;


static inline uint64_t get_bits_raw(void *state)
{
    Xoroshiro64StStState *obj = state;
    const uint32_t s0 = obj->s[0];
    uint32_t s1 = obj->s[1];
    const uint32_t result = rotl32(s0 * 0x9E3779BB, 5) * 5;
    s1 ^= s0;
    obj->s[0] = rotl32(s0, 26) ^ s1 ^ (s1 << 9); // a, b
    obj->s[1] = rotl32(s1, 13); // c

    return result;
}

static void *create(const CallerAPI *intf)
{
    Xoroshiro64StStState *obj = intf->malloc(sizeof(Xoroshiro64StStState));
    uint64_t seed = intf->get_seed64();
    obj->s[0] = (uint32_t) (seed >> 32);
    obj->s[1] = (uint32_t) (seed & 0xFFFFFFFF);
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[0] = 0x12345678;
        obj->s[1] = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT32_PRNG("xoroshiro64**", NULL)
