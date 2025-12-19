/**
 * @file prvhash12cw.c
 * @brief It is a modification of the prvhash64-core chaotic PRNG developed
 * by Aleksey Vaneev.
 * @details It is designed for testing the algorithm quality, not for practical
 * applications.
 *
 * WARNING! It has small average period like 2^47, usage of this generator for
 * statistical, scientific and engineering computations is strongly discouraged!
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


static inline uint16_t trunc12(int32_t x)
{
    return (uint16_t) (x & 0xFFF);
}

static inline uint16_t trunc12u(uint32_t x)
{
    return (uint16_t) (x & 0xFFF);
}

static inline uint16_t PrvHashCore16State_get_bits(PrvHashCore16State *obj)
{
    obj->w = trunc12(obj->w + 0x9E3);
    obj->seed = trunc12u( obj->seed * (obj->lcg * 2U + 1U) );
	const uint16_t rs = trunc12((obj->seed << 6) | (obj->seed >> 6));
    obj->hash  = trunc12(obj->hash + rs + 0xAAA);
    obj->lcg   = trunc12(obj->lcg + obj->seed + obj->w);
    obj->seed ^= obj->hash;
    return obj->lcg ^ rs;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t a = PrvHashCore16State_get_bits(state);
    uint32_t b = PrvHashCore16State_get_bits(state);
    uint32_t c = PrvHashCore16State_get_bits(state);
    return (a << 24) | (b << 12) | c;
}

static void *create(const CallerAPI *intf)
{
    PrvHashCore16State *obj = intf->malloc(sizeof(PrvHashCore16State));
    obj->seed = trunc12u(intf->get_seed32());
    obj->lcg  = trunc12u(intf->get_seed32());
    obj->hash = trunc12u(intf->get_seed32());
    obj->w    = trunc12u(intf->get_seed32());
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}



MAKE_UINT32_PRNG("prvhash-core12-weyl", NULL)
