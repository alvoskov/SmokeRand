/**
 * @file coveyou128.c
 * @brief Coveyou128 PRNG.
 * @details See TAOCP2 chapter 3.2.2
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef Lcg128State Coveyou128State;

static inline uint64_t get_bits_raw(Coveyou128State *obj)
{
    //obj->x = (obj->x + 1) * obj->x;
    uint64_t hi = obj->x_high, lo = obj->x_low;
    unsigned_add128(&hi, &lo, 1);
    umuladd_128x128p64w(obj->x_high, obj->x_low, &hi, &lo, 0);
    obj->x_high = hi; obj->x_low = lo;
    return hi;
}

static void *create(const CallerAPI *intf)
{
    Coveyou128State *obj = intf->malloc(sizeof(Coveyou128State));
    obj->x_high = intf->get_seed64();
    obj->x_low  = (intf->get_seed64() << 2) + 2;  // x0 mod 4 = 2;
    return obj;
}

MAKE_UINT64_PRNG("Coveyou128", NULL)
