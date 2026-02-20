/**
 * @file xorrot128.c
 * @brief xorrot128 is a LFSR with 128-bit state, its period is \f$2^{128} - 1\f$.
 * @details The algorithm is suggested by A. L. Voskov. It uses a reversible
 * operation based on XORs of odd numbers of rotations from [1].
 *
 * Some full period shifts with bad `hamming_distr`/`full`: (11,3,47)/1e-16,
 * (1,5,31)/1e-6, (1,31,41): 1e-6, (31,7,31): 1e-100
 *
 * Some full period shifts with good `hamming distr`/`full`:
 * (1,7,61)/0.001 (bad birthday spacings), (1,17,29)/3e-4 (much better)
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
    obj->x = obj->y;
    obj->y = x0 ^ (x0 << 1) ^ y0 ^ rotl64(y0, 17) ^ rotl64(y0, 29);
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


MAKE_UINT64_PRNG("xorrot128", NULL)
