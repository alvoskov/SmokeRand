/**
 * @file superduper73_shared.c
 * @brief An implementation of 32-bit combined "Super Duper" PRNG
 * by G. Marsaglia.
 * @details 
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief SuperDuper73 PRNG state
 */
typedef struct {
    uint32_t lcg;
    uint32_t xs;
} SuperDuper73State;


static inline uint64_t get_bits_raw(void *state)
{
    SuperDuper73State *obj = state;
    obj->lcg *= 69069;
    obj->xs = obj->xs ^ (obj->xs >> 15);
    obj->xs = obj->xs ^ (obj->xs << 17);
    return obj->lcg ^ obj->xs;
}


static void *create(const CallerAPI *intf)
{
    SuperDuper73State *obj = intf->malloc(sizeof(SuperDuper73State));
    uint64_t seed = intf->get_seed64();
    obj->lcg = (seed & 0xFFFFFFFF) | 0x1;
    obj->xs = seed >> 32;
    if (obj->xs == 0) {
        obj->xs = 0xDEADBEEF;
    }
    return (void *) obj;
}

MAKE_UINT32_PRNG("SuperDuper73", NULL)
