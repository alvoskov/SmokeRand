/**
 * @file xorrot64w16sc.c
 * @brief A xorshift-style LFSR for 16-bit computers that has the
 * \f$2^{64} - 1\f$ period.
 * @details It uses the next step matrix:
 *
 *                  | I 0 I A |
 *     |x y z w | * | I 0 0 0 |
 *                  | 0 I 0 0 |
 *                  | 0 0 I B |
 *
 * Possible triples: (3, 7, 13) and (3, 6, 11) (hamming.cfg and default passed)
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t w;
} Xorrot64w16State;


static inline uint16_t get_bits16(Xorrot64w16State *obj)
{
    const uint16_t x0 = obj->x, w0 = obj->w;
    const uint16_t out = x0;
    obj->x = (uint16_t) (x0 ^ obj->y);
    obj->y = obj->z;
    obj->z = (uint16_t) (x0 ^ w0);
    obj->w = (uint16_t) ((x0 << 3) ^ obj->z ^ rotl16(w0, 7) ^ rotl16(w0, 13));
    return out;
}


static inline uint64_t get_bits_raw(void *state)
{
    const uint32_t hi = get_bits16(state);
    const uint32_t lo = get_bits16(state);
    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Xorrot64w16State *obj = intf->malloc(sizeof(Xorrot64w16State));
    uint64_t seed = intf->get_seed64();
    if (seed == 0) {
        seed = 0x123456789ABCDEF;
    }
    obj->x = (uint16_t) seed;
    obj->y = (uint16_t) (seed >> 16);
    obj->z = (uint16_t) (seed >> 32);
    obj->w = (uint16_t) (seed >> 48);
    return obj;
}


/**
 * @brief An internal self-test based on test vectors independently
 * generated in Python 3.x scripts (see `misc/lfsr/xorrot_gentestvec.py`).
 * These generators are based on explicit matrix arithmetics in GF(2).
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint16_t
        x_ref = 0xf148, y_ref = 0xb4d9, z_ref = 0x44fe, w_ref = 0xa960;

    Xorrot64w16State obj = {
        .x = 0x1234, .y = 0xFEDC, .z = 0xDEAD, .w = 0xBEEF
    };
    for (long i = 0; i < 5000000; i++) {
        (void) get_bits_raw(&obj);
    }
    intf->printf("x_out = %X; x_ref = %X\n",
        (unsigned int) obj.x, (unsigned int) x_ref);
    intf->printf("y_out = %X; y_ref = %X\n",
        (unsigned int) obj.y, (unsigned int) y_ref);
    intf->printf("z_out = %X; z_ref = %X\n",
        (unsigned int) obj.z, (unsigned int) z_ref);
    intf->printf("w_out = %X; w_ref = %X\n",
        (unsigned int) obj.w, (unsigned int) w_ref);
    return (obj.x == x_ref && obj.y == y_ref &&
            obj.z == z_ref && obj.w == w_ref);
}


MAKE_UINT32_PRNG("xorrot64w16", run_self_test)
