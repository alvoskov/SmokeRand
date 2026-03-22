/**
 * @file xorrot128mn.c
 * @brief xorrot128mn is a scrambled version of xorrot128,
 * its period is \f$2^{128} - 1\f$.
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
    uint64_t x;
    uint64_t y;
} Xorrot128State;


static inline uint64_t get_bits_raw(Xorrot128State *obj)
{
    const uint64_t x0 = obj->x, y0 = obj->y;
    const uint64_t out = rotl64(x0 * 6906969069U, 11) - y0;
    obj->x = x0 ^ obj->y;
    obj->y = (x0 << 3) ^ obj->x ^ rotl64(y0, 17) ^ rotl64(y0, 52);
    return out;
}


static void *create(const CallerAPI *intf)
{
    Xorrot128State *obj = intf->malloc(sizeof(Xorrot128State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    if (obj->x == 0) {
        obj->x = 0xDEADBEEF;
    }
    return obj;
}


MAKE_UINT64_PRNG("xorrot128*-", NULL)
