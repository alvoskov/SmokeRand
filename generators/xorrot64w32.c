/**
 * @file xorrot64w32.c
 * @brief xorrot64 is a LFSR with 64-bit state, its period is \f$2^{64} - 1\f$.
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
 * Some explored triples (`hamming_distr` from `full` battery + all `brief`):
 * 
 * - (1 9 31)/1e-161/27 and (3 1 10)/1e-155/27: bad collover
 * - (5 13 25)/24.6e-189/29.3 (no linear artefacts at brief)
 * 
 * Other less explored triples:
 *
 * (5 4 17)/1e-210/31, (5 6 13)/1e-202/30, (5 7 27)/1e-251/33,
 * (5 13 20)/1.92e-204/31, (7 8 29)/1e-236/33, (5 3 15), (5 16 25), (5 17 20),
 * (5 18 25), (7 2 15), (7 3 4), (7 5 10), (7 15 18), (7 17 21), (7 17 28),
 * (7 22 31), (17 11 17)/0/60, (17 11 29)/0/59
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
    obj->x = obj->y;
    obj->y = x0 ^ (x0 << 5) ^ y0 ^ rotl32(y0, 13) ^ rotl32(y0, 25);
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


MAKE_UINT32_PRNG("xorrot64w32", NULL)
