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
 * @brief MTC16 generator state.
 */
typedef struct {
    uint16_t a;
    uint16_t b;
    uint16_t ctr;
} Mtc16State;


static inline uint64_t Mtc16State_get_bits(Mtc16State *obj)
{
    uint16_t old = obj->a + obj->b;
    obj->a = (uint16_t) ( (obj->b * 62317u) ^ ++obj->ctr );
    obj->b = rotl16(old, 10);
    return obj->a;
}


static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = (uint32_t) Mtc16State_get_bits(state);
    uint32_t lo = (uint32_t) Mtc16State_get_bits(state);
    return (hi << 16) | lo;
}



static void *create(const CallerAPI *intf)
{
    Mtc16State *obj = intf->malloc(sizeof(Mtc16State));
    uint64_t seed = intf->get_seed64();
    obj->a = seed & 0xFFFF;
    obj->b = (seed >> 16) & 0xFFFF;
    obj->ctr = (seed >> 32) & 0xFFFF;
    return (void *) obj;
}

MAKE_UINT32_PRNG("Mtc16", NULL)
