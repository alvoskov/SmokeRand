/**
 * @file xorrot128w32.c
 * @brief A xorshift-style LFSR that has the \f$2^{256} - 1\f$ period.
 * @details It uses the next step matrix:
 *
 *                  | I 0 I A |
 *     |x y z w | * | I 0 0 0 |
 *                  | 0 I 0 0 |
 *                  | 0 0 I B |
 *
 * Known triples that pass `hamming.cfg` and `default` (except matrix rank/
 * linear complexity): (1 7 21), (1 15 21), (7 4 21), (9 4 17)
 *
 * Recommended triple: (9, 4, 17) (also passes `full` and Crush/BigCrush except
 * matrix rank/linear complexity tests). Passes PractRand 0.96 at >= 4 TiB
 * except the BRank text.
 *
 * The algorithm is designed by A.L. Voskov.
 *
 * References:
 *
 * 1. Ronald L. Rivest. On the invertibility of the XOR of rotations of
 *    a binary word https://people.csail.mit.edu/rivest/pubs/Riv11e.prepub.pdf
 * 2. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *    V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 * 3. xoshiro / xoroshiro generators and the PRNG shootout
 *    https://prng.di.unimi.it/
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
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (x0 << 9) ^ obj->z ^ rotl32(w0, 4) ^ rotl32(w0, 17);
    return x0;
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

MAKE_UINT32_PRNG("xorrot128w32", run_self_test)
