/**
 * @file prvhash16cw.c
 * @brief It is a modification of the prvhash64-core chaotic PRNG developed
 * by Aleksey Vaneev.
 * @details It is designed for testing the algorithm quality, not for practical
 * applications.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * References:
 *
 * 1. https://github.com/avaneev/prvhash
 *
 * @copyright The prvhash-core algorithm was developed by Aleksey Vaneev.
 *
 * "Weyl sequence" modification and implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t seed;
    uint16_t lcg;
    uint16_t hash;
    uint16_t w;
} PrvHashCore16State;


static inline uint16_t PrvHashCore16State_get_bits(PrvHashCore16State *obj)
{
    obj->w += 0x9E37;
    obj->seed *= (uint16_t) (obj->lcg * 2U + 1U);
	const uint16_t rs = rotl16(obj->seed, 8);
    obj->hash += (uint16_t) (rs + 0xAAAA);
    obj->lcg += (uint16_t) (obj->seed + obj->w);
    obj->seed ^= obj->hash;
    return obj->lcg ^ rs;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = PrvHashCore16State_get_bits(state);
    uint32_t lo = PrvHashCore16State_get_bits(state);
    return (hi << 16) | lo;
}

static void *create(const CallerAPI *intf)
{
    PrvHashCore16State *obj = intf->malloc(sizeof(PrvHashCore16State));
    obj->seed = (uint16_t) intf->get_seed64();
    obj->lcg  = (uint16_t) intf->get_seed64();
    obj->hash = (uint16_t) intf->get_seed64();
    obj->w    = (uint16_t) intf->get_seed64();
    return obj;
}



MAKE_UINT32_PRNG("prvhash-core16-weyl", NULL)
