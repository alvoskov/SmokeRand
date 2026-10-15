/**
 * @file lcg_hp48.c
 * @brief Decimal 48-bit pseudorandom calculator used in HP 48
 * programmable calculator.
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#ifdef __SIZEOF_INT128__

typedef struct {
    __uint128_t x;
} HP48State;

static inline uint64_t get_bits_raw(HP48State *obj)
{
    obj->x = (2851130928467U * obj->x) % 1000000000000000U;
    uint32_t out = (uint32_t) ((double) obj->x * 4.294967296e-06);
    return out;
}

static void *create(const CallerAPI *intf)
{
    HP48State *obj = intf->malloc(sizeof(HP48State));    
    obj->x = intf->get_seed64() | 7;
    return obj;
}

#else

static inline uint64_t get_bits_raw(void *obj)
{
    return 0;
}

static void *create(const CallerAPI *intf)
{
    return NULL;
}

#endif

MAKE_UINT32_PRNG("LCG_HP48", NULL)
