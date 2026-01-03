/**
 * @file loop_antigap_w64.c
 * @brief Just a loop that intended to cause an infinite loop inside the gap
 * test implementation.
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t ctr;
} LoopAntigap64;

static inline uint64_t get_bits_raw(LoopAntigap64 *obj)
{
    obj->ctr++;
    return (obj->ctr > 1000) ? ((1ull << 63ull) - 1ull) : 0;
}


static void *create(const CallerAPI *intf)
{
    LoopAntigap64 *obj = intf->malloc(sizeof(LoopAntigap64));
    obj->ctr = 0;
    return obj;
}

MAKE_UINT64_PRNG("loop_antigap_w64", NULL)
