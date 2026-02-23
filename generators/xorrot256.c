/**
 * @file xorrot256.c
 * @brief A xorshift-style LFSR that has the \f$2^{256} - 1\f$ period.
 * @details It uses the next step matrix:
 *
 *                  | I 0 I A |
 *     |x y z w | * | I 0 0 0 |
 *                  | 0 I 0 0 |
 *                  | 0 0 I B |
 *
 * Known triples:
 *
 * - (3, 8, 37) - passes `hamming.cfg` and >= 8 Tib PractRand 0.96
 *                (except BRank test for binary matrices ranks)
 * - (5, 12, 35) - passes `hamming.cfg`.
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
    uint64_t x;
    uint64_t y;
    uint64_t z;
    uint64_t w;
} Xorrot256State;


static inline uint64_t get_bits_raw(Xorrot256State *obj)
{
    const uint64_t x0 = obj->x, w0 = obj->w;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (x0 << 3) ^ obj->z ^ rotl64(w0, 8) ^ rotl64(w0, 37);
    return x0;
}


static void *create(const CallerAPI *intf)
{
    Xorrot256State *obj = intf->malloc(sizeof(Xorrot256State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64();
    if (obj->w == 0) {
        obj->w = 0x123456789ABCDEF;
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
    static const uint64_t
        x_ref = 0x5e297db916d40034, y_ref=0xcedb4a60b92555bd,
        z_ref = 0x68d87952d71e7344, w_ref=0x7900cabfb34f48cc;
    Xorrot256State obj = {
        .x = 0x123456789ABCDEF, .y = 0xFEDCBA987654321,
        .z = 0xDEADBEEF,        .w = 0xBADF00D
    };
    for (long i = 0; i < 10000000; i++) {
        (void) get_bits_raw(&obj);
    }
    intf->printf("x_out = %llX; x_ref = %llX\n",
        (unsigned long long) obj.x, (unsigned long long) x_ref);
    intf->printf("y_out = %llX; y_ref = %llX\n",
        (unsigned long long) obj.y, (unsigned long long) y_ref);
    intf->printf("z_out = %llX; z_ref = %llX\n",
        (unsigned long long) obj.z, (unsigned long long) z_ref);
    intf->printf("w_out = %llX; w_ref = %llX\n",
        (unsigned long long) obj.w, (unsigned long long) w_ref);
    return (obj.x == x_ref && obj.y == y_ref &&
            obj.z == z_ref && obj.w == w_ref);
}

MAKE_UINT64_PRNG("xorrot256", run_self_test)


