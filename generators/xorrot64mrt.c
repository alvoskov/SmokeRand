/**
 * @file xorrot64mrt.c
 * @brief xorrot64mrt is scrambled version of xorrot64,
 * its period is \f$2^{64} - 1\f$. Uses the same scrambler as
 * xorrot128mrt.
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
} Xorrot64State;


static inline uint64_t get_bits_raw(Xorrot64State *obj)
{
    uint64_t out = rotl64(6906969069U * obj->x, 11);
    out += (out * out | 0x40000005);
    obj->x ^= obj->x << 5;
    obj->x ^= rotl64(obj->x, 13) ^ rotl64(obj->x, 47);
    return out;
}


static void *create(const CallerAPI *intf)
{
    Xorrot64State *obj = intf->malloc(sizeof(Xorrot64State));
    obj->x = intf->get_seed64();
    if (obj->x == 0) {
        obj->x = 0xDEADBEEF;
    }
    return obj;
}

                           
MAKE_UINT64_PRNG("xorrot64mrt", NULL)
