/**
 * @file sfc32_shared.c
 * @brief SFC32 (Small Fast Chaotic 32-bit) PRNG with period at least 2^{32}.
 * @details This generator is one of the fastest because it doesn't use
 * multiplications. It slightly remembers LFSR based generators but includes
 * a lot of additions. Addition is non-linear operation in GF(2) that prevents
 * problem with MatrixRank and LinearComp tests. The theory behind SFC32
 * is not clear.
 *
 * WARNING! MINIMAL PERIOD IS 2^{32}! IT IS NOT ENOUGH FOR RELIABLE PRACTICAL
 * USAGE! Probability to achieve it is low but exactly unknown (even SFC16
 * with 16-bit counter usually passes 32TiB PractRand; but bad seeds are
 * possible).
 *
 * SFC32 passes `brief`, `default` and `full` batteries.
 * It also passes BigCrush (TestU01) and PractRand.
 *
 * @copyright SFC32 algorithm is developed by Chris Doty-Humphrey,
 * the author of PractRand (https://sourceforge.net/projects/pracrand/).
 * Some portions of the source code were taken from PractRand that is
 * released as Public Domain.
 * 
 * Adaptation for SmokeRand:
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t counter;
} Sfc32State;

static inline uint64_t get_bits_raw(void *state)
{
    Sfc32State *obj = state;
    enum {BARREL_SHIFT = 21, RSHIFT = 9, LSHIFT = 3};
    uint32_t tmp = obj->a + obj->b + obj->counter++;
    obj->a = obj->b ^ (obj->b >> RSHIFT);
    obj->b = obj->c + (obj->c << LSHIFT);
    obj->c = ((obj->c << BARREL_SHIFT) | (obj->c >> (32-BARREL_SHIFT))) + tmp;
    return tmp;
}

void *create(const CallerAPI *intf)
{
    Sfc32State *obj = intf->malloc(sizeof(Sfc32State));
    uint64_t s = intf->get_seed64();
	obj->a = 0; // a gets mixed in the slowest
    obj->b = (uint32_t) s;
    obj->c = (uint32_t) (s >> 32);
    obj->counter = 1;
    for (int i = 0; i < 12; i++) {
        get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("SFC32", NULL)
