/**
 * @file superduper64_u32.c
 * @brief An implementation of 64-bit combined "Super Duper" PRNG
 * by G. Marsaglia.
 * @details 
 *
 * https://groups.google.com/g/comp.sys.sun.admin/c/GWdUThc_JUg/m/_REyWTjwP7EJ
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "superduper64_body.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
    return superduper64_get_bits(state) >> 32;
}

static void *create(const CallerAPI *intf)
{
    return superduper64_create(intf);
}

MAKE_UINT32_PRNG("SuperDuper64_u32", NULL)
