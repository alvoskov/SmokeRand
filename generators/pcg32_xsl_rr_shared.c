/**
 * @file pcg32_xsl_rr_shared.c
 * @brief PCG32_XSL_RR generator: PCG modification based on 64-bit LCG
 * that uses XSL-RR output function.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} Lcg64State;

static inline uint64_t get_bits_raw(void *state)
{
    Lcg64State *obj = state;
    obj->x = obj->x * 6906969069 + 1;
    return rotl32((obj->x >> 32) ^ (obj->x & 0xFFFFFFFF), obj->x >> 58);
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT32_PRNG("PCG32_XSL_RR", NULL)
