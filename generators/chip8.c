/**
 * @file chip8.c
 * @brief Chip8 is a 8-bit modification of Chip32 chaotic PRNG.
 * @details The original Chip32 was suggested by Tommy Ettinger, it is a very
 * fast chaotic generator. Chip8 is its 8-bit version made by A.L. Voskov
 * using the SmokeRand `hamming_distr` test for rotations tuning (the used
 * optimization technique is described in Chip64 documentation).
 *
 * WARNING! The minimial guaranteed period is only 2^{8}! It is not enough for
 * reliable practical usage, bad seeds are possible. The average period is
 * likely close to \f$ 2^{31} \f$. Consider `chip8` as a toy generator that was
 * made mainly to check robustness of the `chip` family design.
 *
 * References:
 *
 * - https://github.com/tommyettinger/juniper/blob/main/src/main/java/com/github/tommyettinger/random/Chip32Random.java
 *
 * @copyright The Chip32 algorithm was suggested by Tommy Ettinger.
 * 
 * Chip8 modification:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
} Chip8State;


static inline uint8_t Chip8_get_bits(Chip8State *obj)
{
    const uint8_t a = obj->a, b = obj->b, c = obj->c, d = obj->d;
    obj->a = (uint8_t) (b + c);
    obj->b = (uint8_t) (d ^ a);
    obj->c = rotl8(b, 2);
    obj->d = (uint8_t) (obj->d + 0x9DU);
    return rotl8(a, 2) ^ (rotl8(b, 7) + c);
}


static inline uint64_t get_bits_raw(void *state)
{
    uint32_t x = 0;
    for (int i = 0; i < 4; i++) {
        x <<= 8;
        x |= Chip8_get_bits(state);
    }
    return x;
}


static void *create(const CallerAPI *intf)
{
    Chip8State *obj = intf->malloc(sizeof(Chip8State));
    const uint64_t seed = intf->get_seed64();
    obj->a = (uint8_t) (seed >> 24);
    obj->b = (uint8_t) (seed >> 16);
    obj->c = (uint8_t) (seed >> 8);
    obj->d = (uint8_t) seed;
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("chip8", NULL)
