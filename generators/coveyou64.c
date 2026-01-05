/**
 * @file coveyou64.c
 * @brief Coveyou64 PRNG.
 * @details Passes SmallCrush but fails the next two tests
 * from Crush:
 *
 * - 17  BirthdaySpacings, t = 8
 * - 26  SimpPoker, d = 64
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} Coveyou64State;

static inline uint64_t get_bits_raw(Coveyou64State *obj)
{
    obj->x = (obj->x + 1) * obj->x;
    return obj->x >> 32;
}

static void *create(const CallerAPI *intf)
{
    Coveyou64State *obj = intf->malloc(sizeof(Coveyou64State));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("Coveyou64", NULL)
