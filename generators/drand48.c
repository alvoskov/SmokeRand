/**
 * @file drand48.c
 * @brief Classical 48-bit LCG that returns the upper 32 bits used in
 * the `drand48` function. Fails a lot of statistical tests.
 * @details References:
 *
 * 1. https://pubs.opengroup.org/onlinepubs/7908799/xsh/drand48.html
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
} Lcg48State;

static inline uint64_t get_bits_raw(Lcg48State *obj)
{
    static const uint64_t mask = (1ull << 48) - 1;
    obj->x = (obj->x * 0x5DEECE66D + 0xB) & mask;
    return obj->x >> 16;
}

static void *create(const CallerAPI *intf)
{
    Lcg48State *obj = intf->malloc(sizeof(Lcg48State));
    obj->x = intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("drand48", NULL)
