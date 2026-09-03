/**
 * @file xorrot512mrt.c
 * @brief A xorshift-style LFSR that has the \f$2^{512} - 1\f$ period.
 * @details It is a scrambled version xorrot512mrt
 *
 * The algorithm is designed by A.L. Voskov.
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
    uint64_t x[8];
} Xorrot512State;


static inline uint64_t get_bits_raw(Xorrot512State *obj)
{
    static const int a = 15, b = 27, c = 46;
    const uint64_t x0 = obj->x[0], x7 = obj->x[7];
    uint64_t out = rotl64(6906969069U * x0, 11);
    out += (out * out | 0x40000005);
    obj->x[0] = x0 ^ obj->x[1];
    obj->x[1] = obj->x[2];
    obj->x[2] = obj->x[3];
    obj->x[3] = obj->x[4];
    obj->x[4] = obj->x[5];
    obj->x[5] = obj->x[6];
    obj->x[6] = x0 ^ x7;
    obj->x[7] = (x0 << a) ^ obj->x[6] ^ rotl64(x7, b) ^ rotl64(x7, c);
    return out;
}

static void *create(const CallerAPI *intf)
{
    Xorrot512State *obj = intf->malloc(sizeof(Xorrot512State));
    for (int i = 0; i < 8; i++) {
        obj->x[i] = intf->get_seed64();
    }
    if (obj->x[0] == 0) {
        obj->x[0] = 0x123456789ABCDEF;
    }
    return obj;
}


MAKE_UINT64_PRNG("xorrot512mrt", NULL)
