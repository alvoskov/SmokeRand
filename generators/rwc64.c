/**
 * @file rwc64.c
 * @brief
 * @details Period is around 2^244
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
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x; ///< x_(n-1)
    uint64_t y; ///< x_(n-2)
    uint64_t z; ///< x_(n-3)
    uint64_t c; ///< carry
} Rwc64State;



static inline uint64_t get_bits_raw(Rwc64State *obj)
{
    static const uint64_t a = 12345671234567586U;
    uint64_t new_hi = 0, new_lo = obj->y;
    unsigned_add128(&new_hi, &new_lo, obj->z);
    umuladd_128x128p64w(0, a, &new_hi, &new_lo, obj->c);
    obj->z = obj->y;
    obj->y = obj->x;
    obj->x = new_lo;
    obj->c = new_hi;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Rwc64State *obj = intf->malloc(sizeof(Rwc64State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->c = 1;
    return obj;
}

MAKE_UINT64_PRNG("rwc64", NULL)
