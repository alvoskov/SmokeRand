/**
 * @file xorrot64w32mn.c
 * @brief xorrot64w32mn is a scrambler version of xorrot64w32,
 * its period is \f$2^{64} - 1\f$.
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
    uint32_t x;
    uint32_t y;
} Xorrot64w32State;


static inline uint64_t get_bits_raw(Xorrot64w32State *obj)
{
    const uint32_t x0 = obj->x, y0 = obj->y;
    const uint32_t out = rotl32(x0 * 69069U, 5) - y0;
    obj->x = obj->y;
    obj->y = x0 ^ (x0 << 5) ^ y0 ^ rotl32(y0, 13) ^ rotl32(y0, 25);
    return out;
}


static void *create(const CallerAPI *intf)
{
    Xorrot64w32State *obj = intf->malloc(sizeof(Xorrot64w32State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    if (obj->x == 0) {
        obj->x = 0xDEADBEEF;
    }
    return obj;
}


MAKE_UINT32_PRNG("xorrot64w32*-", NULL)
