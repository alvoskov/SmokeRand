/**
 * @file xorrot64w32.c
 * @brief xorrot64w32 is a LFSR with 64-bit state consisting of two 32-bit
 * unsigned integers, its period is \f$2^{64} - 1\f$.
 * @details The algorithm is suggested by A. L. Voskov. It uses a reversible
 * operation based on XORs of odd numbers of rotations from [1].
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
 * - Good triple: (5,12,25) but fails bspace16_4d in `full`
 * - Probably acceptable triples (worse in HW tests): (3,8,29), (5,15,24)
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
    obj->x = x0 ^ obj->y;
    obj->y = (x0 << 5) ^ obj->x ^ rotl32(y0, 12) ^ rotl32(y0, 25);
    return x0;
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


/**
 * @brief An internal self-test based on test vectors independently
 * generated in Python 3.x scripts (see `misc/lfsr/xorrot_gentestvec.py`).
 * These generators are based on explicit matrix arithmetics in GF(2).
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t x_ref = 0xc5b70c13, y_ref = 0xa696794a;
    Xorrot64w32State obj = {.x = 0x12345678, .y = 0xFEDCBA98};
    for (long i = 0; i < 10000000; i++) {
        (void) get_bits_raw(&obj);
    }
    intf->printf("x_out = %lX; x_ref = %lX\n",
        (unsigned long) obj.x, (unsigned long) x_ref);
    intf->printf("y_out = %lX; y_ref = %lX\n",
        (unsigned long) obj.y, (unsigned long) y_ref);
    return (obj.x == x_ref && obj.y == y_ref);
}


MAKE_UINT32_PRNG("xorrot64w32", run_self_test)
