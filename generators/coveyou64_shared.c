/**
 * @file coveyou64_shared.c
 * @brief Coveyou64 PRNG.
 * @details Passes SmallCrush but fails the next two tests
 * from Crush:
 *
 * - 17  BirthdaySpacings, t = 8
 * - 26  SimpPoker, d = 64
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 *
 * In scientific publications which used this software, a reference to it
 * would be appreciated.
 */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} Coveyou64State;

static uint64_t get_bits(void *state)
{
    Coveyou64State *obj = (Coveyou64State *) state;
    obj->x = (obj->x + 1) * obj->x;
    return obj->x >> 32;
}

static void *create(const CallerAPI *intf)
{
    Coveyou64State *obj = intf->malloc(sizeof(Coveyou64State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT32_PRNG("Coveyou64", NULL)
