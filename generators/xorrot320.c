/**
 * @file xorrot320.c
 * @brief A xorshift-style LFSR that has the \f$2^{320} - 1\f$ period.
 * @details It uses the next step matrix:
 *
 *                    | I 0 0 I A |
 *     |x y z w v | * | I 0 0 0 0 |
 *                    | 0 I 0 0 0 |
 *                    | 0 0 I 0 0 |
 *                    | 0 0 0 I B |
 *
 * The recommended triple is `[11,9,43]`:
 *
 * - passes the `full` battery
 * - >= 40 TB in Vigna HWD test
 * - >= 8 TiB in PractRand except the `BRank` test
 *
 * Other fairly good triples that pass the `full` battery:
 *
 * - [1 25 55]  >= 4TB   (Vigna HWD)
 * - [7 9 60]   >= 2TB   (Vigna HWD)
 * - [9 6 43]   >= 2TB   (Vigna HWD)
 * - [13 25 38] >= 2.5TB (Vigna HWD)
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
    uint64_t v;
} Xorrot320State;


static inline uint64_t get_bits_raw(Xorrot320State *obj)
{
    static const int a = 11, b = 9, c = 43;
    const uint64_t x0 = obj->x, v0 = obj->v;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = x0 ^ v0;
    obj->v = (x0 << a) ^ obj->w ^ rotl64(v0, b) ^ rotl64(v0, c);
    return x0;
}

static void *create(const CallerAPI *intf)
{
    Xorrot320State *obj = intf->malloc(sizeof(Xorrot320State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64();
    obj->v = intf->get_seed64();
    if (obj->v == 0) {
        obj->v = 0x123456789ABCDEF;
    }
    return obj;
}


MAKE_UINT64_PRNG("xorrot320", NULL)
