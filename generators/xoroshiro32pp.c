/**
 * @file xoroshiro32pp.c
 * @brief xoroshiro32++ is a modification of xoroshiro++ with 32-bit state.
 * @details The parameters were taken from [1,2]. The generator has a very
 * short period around \f$ 2^{32} \f$ and shouldn't be used as a general
 * purpose generator. However, it is much better than LCG69069, minstd or
 * xorshift32 and still may be useful for retrocomputing and microcontrollers.
 *
 * References:
 *
 * 1. https://forums.parallax.com/discussion/comment/1448759/#Comment_1448759
 * 2. https://github.com/ZiCog/xoroshiro/blob/master/src/main/c/xoroshiro.h
 * 3. https://groups.google.com/g/prng/c/Ll-KDIbpO8k/m/bfHK4FlUCwAJ
 * 4. https://forums.parallax.com/discussion/comment/1423789/#Comment_1423789
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t s[2];    
} Xoroshiro32PPState;


static inline uint16_t rotl16(uint16_t x, int r)
{
    return (x << r) | (x >> (16 - r));
}

static inline uint16_t Xoroshiro32PPState_get_bits(Xoroshiro32PPState *obj)
{
    const uint16_t s0 = obj->s[0];
    uint16_t s1 = obj->s[1];
    const uint16_t result = rotl16(s0 + s1, 9) + s0;
    s1 ^= s0;
    obj->s[0] = rotl16(s0, 13) ^ s1 ^ (s1 << 5); // a, b
    obj->s[1] = rotl16(s1, 10); // c
    return result;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = Xoroshiro32PPState_get_bits(state);
    uint32_t lo = Xoroshiro32PPState_get_bits(state);
    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Xoroshiro32PPState *obj = intf->malloc(sizeof(Xoroshiro32PPState));
    uint64_t seed = intf->get_seed64();
    obj->s[0] = seed >> 16;
    obj->s[1] = seed & 0xFFFF;
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[0] = 0xDEAD;
        obj->s[1] = 0xBEEF;
    }
    return obj;
}

MAKE_UINT32_PRNG("xoroshiro32++", NULL)
