/**
 * @file pcg32_dxsm.c
 * @brief PCG32-DXSM (Double xor shift multiply) pseudorandom number generator.
 * @details It is a 32-bit modification of the PCG64-DXSM generator suggested
 * by Melissa O'Neill in 2019.
 *
 * References:
 *
 * https://github.com/numpy/numpy/issues/13635#issuecomment-506088698
 * @copyright The PCG-DXSM family of algorithms was suggested by M.O'Neill.
 *
 * Constants tuning and reentrant version for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    static const uint64_t a = 6906969069ull;
    // Just ordinary 128-bit LCG
    obj->x = a * obj->x + 1;
    // Output DXSM (double xor, shift, multiply) function
    uint32_t high = (uint32_t) (obj->x >> 32), low = (uint32_t) obj->x | 0x1;
    high ^= high >> 16;
    high *= 69069u;
    high ^= high >> 24;
    return high * low;
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("PCG32-DXSM", NULL)
