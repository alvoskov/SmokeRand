/**
 * @file xorrot160.c
 * @brief A xorshift-style LFSR that has the \f$2^{160} - 1\f$ period.
 * @details It uses the next step matrix:
 *
 *                    | I 0 0 I A |
 *     |x y z w v | * | I 0 0 0 0 |
 *                    | 0 I 0 0 0 |
 *                    | 0 0 I 0 0 |
 *                    | 0 0 0 I B |
 *
 * The algorithm is designed by A.L. Voskov.
 *
 * The recommended triple is `[7 24 27]`:
 *
 * - passes the `full` battery
 * - passes >= 2 TiB in PractRand 0.96 (except the `BRank` test)
 * - fails MatrixRank/LinearComp tests in SmallCrush (L=60) and Crush
 *
 * Another `[3 19 31]`
 *
 * - passes the `full` battery
 * - fails the BCFN test at 1 TiB in PractRand 0.96
 * - passes SmallCrush, fails Crush (matrix rank for L > 60, linearcomp)
 *
 * Another `[13 4 11]`
 *
 * - passes the `full` battery
 * - passes >= 4 TiB in PractRand 0.96 (except the `Brank` test)
 * - passes SmallCrush, fails Crush and BigCrush (matrix rank for L > 60,
 *   linearcomp, other tests are passed)
 *
 * Acceptable triples (pass default and hamming.cfg/Hamming weight tests from full):
 *
 *     [ 3 19 31] [ 5  5 15] [ 7 24 27] [13  4 11]
 *     [13 11 23] [15  3 25] [17 17 27] [17 19 31]
 *     [19  2 15] [19  6 31] [23  9 24] [25 11 16]
 *     [29 23 29]
 *
 * Bad triple: `[13 11 13]` (susp. gap16_count0), 
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
    uint32_t v;
} Xorrot160State;


static inline uint32_t get_bits_raw(Xorrot160State *obj)
{
    static const int a = 13, b = 4, c = 11;
    const uint32_t x0 = obj->x, v0 = obj->v;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = x0 ^ v0;
    obj->v = (x0 << a) ^ obj->w ^ rotl32(v0, b) ^ rotl32(v0, c);
    return x0;
}

static void *create(const CallerAPI *intf)
{
    Xorrot160State *obj = intf->malloc(sizeof(Xorrot160State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32();
    obj->v = intf->get_seed32();
    if (obj->v == 0) {
        obj->v = 0x12345678;
    }
    return obj;
}


MAKE_UINT32_PRNG("xorrot160", NULL)
