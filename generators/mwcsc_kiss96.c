/**
 * @file mwcsc_kiss96.c
 * @brief MWC generator taken from KISS96 generator (the version from
 * DIEHARD test suite) equipped with an output scrambler. Passes:
 * - `express`, `brief`, `default` and `full` batteries
 * - PractRand 0.94 at least for >= 2 TiB
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
    uint32_t z; ///< MWC state: value.
    uint32_t w; ///< MWC state: value.
    uint32_t c; ///< MWC state: carry.
} MwcKiss96State;



static void MwcKiss96State_init(MwcKiss96State *obj, uint64_t seed)
{
    obj->z = (uint32_t) (seed & 0xFFFFFFFF);
    obj->w = (uint32_t) (seed >> 32);
    obj->c = 1;
}


static inline uint64_t get_bits_raw(MwcKiss96State *obj)
{
    const uint64_t m = 2U*(uint64_t)obj->w + (uint64_t)obj->z + (uint64_t)obj->c;
    obj->z = obj->w;
    obj->w = (uint32_t)m;
    obj->c = (uint32_t)(m >> 32);

    uint32_t out = obj->z + (obj->z * obj->z | 0x4005);
    out = out ^ rotl32(out, 7) ^ rotl32(out, 23);
    return out;
}

static void *create(const CallerAPI *intf)
{
    MwcKiss96State *obj = intf->malloc(sizeof(MwcKiss96State));
    MwcKiss96State_init(obj, intf->get_seed64());
    return obj;
}


MAKE_UINT32_PRNG("MWC_KISS96", NULL)
