/**
 * @file rwc64.c
 * @brief
 * @details Period is around 2^93
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
    uint32_t c;
} Rwc32SmState;



static inline uint64_t get_bits_raw(Rwc32SmState *obj)
{
    const uint64_t new = 1111111464ULL*((uint64_t)obj->x + obj->y) + obj->c;
    obj->y = obj->x;
    obj->x = (uint32_t) new;
    obj->c = (uint32_t) (new >> 32);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Rwc32SmState *obj = intf->malloc(sizeof(Rwc32SmState));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->c = 1;
    return obj;
}

MAKE_UINT32_PRNG("rwc32sm", NULL)
