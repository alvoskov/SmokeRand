/**
 * @file xorrot128.c
 * @brief xorrot128 is a LFSR with 128-bit state, its period is \f$2^{128} - 1\f$.
 * @details The algorithm is suggested by A. L. Voskov. It uses a reversible
 * operation based on XORs of odd numbers of rotations from [1].
 *
 * The next binary matrix is used:
 *
 *    |x w| * | I A |
 *            | I B |
 *
 * Possible triples: (3,17,52) and (2,27,57)
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
} Xorrot128State;


static inline uint64_t get_bits_raw(Xorrot128State *obj)
{
    const uint64_t x0 = obj->x, y0 = obj->y;
    obj->x = x0 ^ obj->y;
    obj->y = (x0 << 3) ^ obj->x ^ rotl64(y0, 17) ^ rotl64(y0, 52);
    return x0;
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

/**
 * @brief An internal self-test based on test vectors independently
 * generated in Python 3.x scripts (see `misc/lfsr/xorrot_gentestvec.py`).
 * These generators are based on explicit matrix arithmetics in GF(2).
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint64_t x_ref = 0xd81c2efae4247c15, y_ref = 0x7898f6b63ff4640a;
    Xorrot128State obj = {.x = 0x123456789ABCDEF, .y = 0xFEDCBA987654321};
    for (long i = 0; i < 10000000; i++) {
        (void) get_bits_raw(&obj);
    }
    intf->printf("x_out = %llX; x_ref = %llX\n",
        (unsigned long long) obj.x, (unsigned long long) x_ref);
    intf->printf("y_out = %llX; y_ref = %llX\n",
        (unsigned long long) obj.y, (unsigned long long) y_ref);
    return (obj.x == x_ref && obj.y == y_ref);
}


MAKE_UINT64_PRNG("xorrot128", run_self_test)
