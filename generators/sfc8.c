/**
 * @file sfc8.c
 * @brief SFC8 (Small Fast Chaotic 8-bit) PRNG with period at least 2^{8}.
 * @details An experimental PRNG designed for searching flaws in SFC16,
 * SFC32 and SFC64.
 *
 * https://gist.github.com/imneme/f1f7821f07cf76504a97f6537c818083
 *
 * @copyright SFC32/64 algorithms are developed by Chris Doty-Humphrey,
 * the author of PractRand (https://sourceforge.net/projects/pracrand/).
 * Some portions of the source code were taken from PractRand that is
 * released as Public Domain.
 *
 * SFC8 "toy" modification was suggested by M.E. O'Neill.
 * 
 * Adaptation for SmokeRand:
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
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
    uint8_t counter;
} Sfc8State;

static inline uint8_t get_bits8_raw(void *state)
{
    Sfc8State *obj = state;
    enum {BARREL_SHIFT = 3, RSHIFT = 2, LSHIFT = 1};
    const uint8_t tmp = (uint8_t) (obj->a + obj->b + obj->counter++);
    obj->a = (uint8_t) (obj->b ^ (obj->b >> RSHIFT));
    obj->b = (uint8_t) (obj->c + (obj->c << LSHIFT));
    obj->c = (uint8_t) (rotl8(obj->c, BARREL_SHIFT) + tmp);
    return tmp;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t out = get_bits8_raw(state); out <<= 8;
    out |= get_bits8_raw(state); out <<= 8;
    out |= get_bits8_raw(state); out <<= 8;
    out |= get_bits8_raw(state);
    return out;
}

void *create(const CallerAPI *intf)
{
    Sfc8State *obj = intf->malloc(sizeof(Sfc8State));
    uint64_t s = intf->get_seed64();
	obj->a = 0; // a gets mixed in the slowest
    obj->b = (uint8_t) s;
    obj->c = (uint8_t) (s >> 32);
    obj->counter = 1;
    for (int i = 0; i < 12; i++) {
        
        intf->printf("%.8llX\n", get_bits_raw(obj));
    }
    return obj;
}

MAKE_UINT32_PRNG("SFC8", NULL)
