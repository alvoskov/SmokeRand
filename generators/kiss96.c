/**
 * @file kiss96.c
 * @brief KISS generator: the version from DIEHARD test suite.
 * @details This version of KISS generator was suggested by G.Marsaglia
 * and included in his DIEHARD test suite. It is a combined generator
 * made from 32-bit LCG ("69069"), xorshift32 and some generalized
 * multiply-with-carry PRNG:
 *
 * \f[
 * z_{n} = 2z_{n-1} + z_{n-2} + c_{n-1} \mod 2^{32}
 * \f]
 *
 * The algorithm for this MWC is taken from DIEHARD FORTRAN source code
 * and uses some bithacks to implement it without 64-bit data types.
 *
 * @copyright The KISS96 algorithm was developed by George Marsaglia.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t x; ///< LCG state.
    uint32_t y; ///< SHR3 state.
    uint32_t z; ///< MWC state: value.
    uint32_t w; ///< MWC state: value.
    uint32_t c; ///< MWC state: carry.
} Kiss96State;



static void Kiss96State_init(Kiss96State *obj, uint64_t seed)
{
    const uint32_t seed_lo = (uint32_t) seed;
    const uint32_t seed_hi = (uint32_t) (seed >> 32);
    obj->x = seed_lo;
    obj->y = seed_hi;
    if (obj->y == 0){
        obj->y = 0x12345678;
    }
    obj->z = (uint32_t) (seed_hi & 0xFFFF);
    obj->w = (uint32_t) (seed_hi >> 16);
    obj->c = 0;
}


static inline uint64_t get_bits_raw(Kiss96State *obj)
{
    obj->x = obj->x * 69069u + 1u;
    obj->y ^= obj->y << 13;
    obj->y ^= obj->y >> 17;
    obj->y ^= obj->y << 5;
    uint32_t k = (obj->z >> 2) + (obj->w >> 3) + (obj->c >> 2);
    uint32_t m = obj->w + obj->w + obj->z + obj->c;
    obj->z = obj->w;
    obj->w = m;
    obj->c = k >> 30;
    return obj->x + obj->y + obj->w;
}

static void *create(const CallerAPI *intf)
{
    Kiss96State *obj = intf->malloc(sizeof(Kiss96State));
    Kiss96State_init(obj, intf->get_seed64());
    return obj;
}

MAKE_UINT32_PRNG("KISS96", NULL)
