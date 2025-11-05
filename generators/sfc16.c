/**
 * @file sfc16.c
 * @brief SFC16 (Small Fast Chaotic 16-bit) PRNG with period at least 2^{16}.
 * @details This generator is one of the fastest because it doesn't use
 * multiplications. It slightly remembers LFSR based generators but includes
 * a lot of additions. Addition is non-linear operation in GF(2) that prevents
 * problem with MatrixRank and LinearComp tests. The theory behind SFC16
 * is not clear.
 *
 * WARNING! MINIMAL PERIOD IS 2^{16}! IT IS NOT ENOUGH FOR RELIABLE PRACTICAL
 * USAGE! It also fails PractRand 0.94 only at 256 GiB of data, but only if its
 * output is processed as a sequence of 32-bit words (`stdin32`).
 *
 * @copyright SFC16 algorithm is developed by Chris Doty-Humphrey,
 * the author of PractRand (https://sourceforge.net/projects/pracrand/).
 * Some portions of the source code were taken from PractRand that is
 * released as Public Domain.
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
    uint16_t a;
    uint16_t b;
    uint16_t c;
    uint16_t counter;
} Sfc16State;

static inline uint16_t get_bits16_raw(void *state)
{
    Sfc16State *obj = state;
    enum {BARREL_SHIFT = 6, RSHIFT = 5, LSHIFT = 3};
    uint16_t tmp = (uint16_t) (obj->a + obj->b + obj->counter++);
    obj->a = (uint16_t) (obj->b ^ (obj->b >> RSHIFT));
    obj->b = (uint16_t) (obj->c + (obj->c << LSHIFT));
    obj->c = (uint16_t) (((obj->c << BARREL_SHIFT) | (obj->c >> (16-BARREL_SHIFT))) + tmp);
    return tmp;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t out = get_bits16_raw(state); out <<= 16;
    out |= get_bits16_raw(state);
    return out;
}


void *create(const CallerAPI *intf)
{
    Sfc16State *obj = intf->malloc(sizeof(Sfc16State));
    uint64_t s = intf->get_seed64();
	obj->a = 0; // a gets mixed in the slowest
    obj->b = (uint16_t) s;
    obj->c = (uint16_t) (s >> 32);
    obj->counter = 1;
    for (int i = 0; i < 12; i++) {
        get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("SFC16", NULL)
