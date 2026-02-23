/**
 * @file xorrot128w32.c
 * @brief A scrambled version of xorrot128w32.
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
    uint32_t z;
    uint32_t w;
} Xorrot128w32State;


static inline uint64_t get_bits_raw(Xorrot128w32State *obj)
{
    const uint32_t x0 = obj->x, w0 = obj->w;
    uint32_t out = rotl32(69069U * x0, 5);
    out += (out * out | 0x4005);
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (x0 << 9) ^ obj->z ^ rotl32(w0, 4) ^ rotl32(w0, 17);
    return out;
}


static void *create(const CallerAPI *intf)
{
    Xorrot128w32State *obj = intf->malloc(sizeof(Xorrot128w32State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32();
    if (obj->w == 0) {
        obj->w = 0x12345678;
    }
    return obj;
}

/**
 * @brief An internal self-test based on test vectors independently
 * generated in Python 3.x scripts (see `misc/lfsr/xorrot_gentestvec.py`).
 * These generators are based on explicit matrix arithmetics in GF(2).
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t
        x_ref = 0x7246b061, y_ref = 0x9c9af12a,
        z_ref = 0x30c898d5, w_ref = 0xbe6f788b;
    Xorrot128w32State obj = {
        .x = 0x12345678, .y = 0xFEDCBA98, .z = 0xDEADBEEF, .w = 0xBADF00D
    };
    for (long i = 0; i < 10000000; i++) {
        (void) get_bits_raw(&obj);
    }
    intf->printf("x_out = %lX; x_ref = %lX\n",
        (unsigned long) obj.x, (unsigned long) x_ref);
    intf->printf("y_out = %lX; y_ref = %lX\n",
        (unsigned long) obj.y, (unsigned long) y_ref);
    intf->printf("z_out = %lX; z_ref = %lX\n",
        (unsigned long) obj.z, (unsigned long) z_ref);
    intf->printf("w_out = %lX; w_ref = %lX\n",
        (unsigned long) obj.w, (unsigned long) w_ref);
    return (obj.x == x_ref && obj.y == y_ref &&
            obj.z == z_ref && obj.w == w_ref);
}

MAKE_UINT32_PRNG("xorrot128w32mrt", run_self_test)
