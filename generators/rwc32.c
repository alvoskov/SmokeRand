/**
 * @file rwc64.c
 * @brief
 * @details Period is around 2^125, it was a modification initially
 * made by A.L. Voskov an error (-2/-3 instead -1/-2 lags) but it
 * showed good empirical properties.
 *
 * References:
 *
 * 1. https://www.stat.berkeley.edu/~spector/s243/mother.c
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t c;
} Rwc32State;



static inline uint64_t get_bits_raw(Rwc32State *obj)
{
    const uint64_t new = 1111111464ULL*((uint64_t)obj->y + obj->z) + obj->c;
    obj->z = obj->y;
    obj->y = obj->x;
    obj->x = (uint32_t) new;
    obj->c = (uint32_t) (new >> 32);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Rwc32State *obj = intf->malloc(sizeof(Rwc32State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->c = 1;
    return obj;
}

MAKE_UINT32_PRNG("rwc32", NULL)
