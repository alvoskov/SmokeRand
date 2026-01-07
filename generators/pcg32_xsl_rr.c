/**
 * @file pcg32_xsl_rr.c
 * @brief PCG32_XSL_RR generator: PCG modification based on 64-bit LCG
 * that uses XSL-RR output function.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    obj->x = obj->x * 6906969069ull + 1ull;
    const uint32_t xored = (uint32_t) ((obj->x >> 32) ^ (obj->x & 0xFFFFFFFF));
    return rotr32(xored, (int) (obj->x >> 58));
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("PCG32_XSL_RR", NULL)
