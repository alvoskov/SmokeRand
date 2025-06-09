/**
 * @file weyl.c
 * @brief 64-bit discrete Weyl sequence.
 * @details Fails almost all statistical tests.
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
    Lcg64State *obj = state;
    obj->x += 0x9E3779B97F4A7C15;
    return obj->x >> 32;
}

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT32_PRNG("Weyl", NULL)
