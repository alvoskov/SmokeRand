/**
 * @file mtc16.c
 * @brief A very fast multiplication-based chaotic PRNG by Chris Doty-Humphrey.
 * @details Note: the parameters of 16-bit version were not published by the
 * author. They were tuned by A.L. Voskov using PractRand 0.94 and Knuth's
 * spectral test for the multiplier. References:
 *
 * 1. https://sourceforge.net/p/pracrand/discussion/366935/thread/f310c67275/
 *
 * This MTC16 version fails PractRand 0.94 at 512 GiB (stdin32) or
 * at 256 GiB (stdin16).
 *
 * @copyright MTC16 algorithm was developed by Chris Doty-Humphrey,
 * the author of PractRand.
 *
 * Parameters tuning and reentrant implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MTC8 generator state.
 */
typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t ctr;
} Mtc8State;


static inline uint8_t Mtc8State_get_bits(Mtc8State *obj)
{
    uint8_t old = obj->a + obj->b;
    obj->a = (uint8_t) ((obj->b * 123u) ^ ++obj->ctr);
    obj->b = rotl8(old, 3);
    return obj->a;
}


static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = Mtc8State_get_bits(state);
    }
    return out.u32;
}



static void *create(const CallerAPI *intf)
{
    Mtc8State *obj = intf->malloc(sizeof(Mtc8State));
    uint64_t seed = intf->get_seed64();
    obj->a = seed & 0xFF;
    obj->b = (seed >> 8) & 0xFF;
    obj->ctr = (seed >> 16) & 0xFF;
    return (void *) obj;
}

MAKE_UINT32_PRNG("Mtc8", NULL)
