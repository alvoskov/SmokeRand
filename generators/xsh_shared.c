/**
 * @file xsh_shared.c
 * @brief An implementation of 64-bit LSFR generator proposed by G. Marsaglia.
 * @details This version of "xorshift" generator was included in KISS99 PRNG.
 * It fails SmallCrush:
 *
 *           Test                          p-value
 *     ----------------------------------------------
 *      8  MatrixRank                       eps
 *     10  RandomWalk1 H                    eps
 *     10  RandomWalk1 M                   6.1e-5
 *     ----------------------------------------------
 *     All other tests were passed
 *
 * It fails Crush:
 *
 *           Test                          p-value
 *     ----------------------------------------------
 *     14  BirthdaySpacings, t = 7        3.5e-14
 *     15  BirthdaySpacings, t = 7        1.2e-10
 *     17  BirthdaySpacings, t = 8        2.5e-10
 *     56  MatrixRank, 60 x 60              eps
 *     57  MatrixRank, 60 x 60              eps
 *     58  MatrixRank, 300 x 300            eps
 *     59  MatrixRank, 300 x 300            eps
 *     60  MatrixRank, 1200 x 1200          eps
 *     61  MatrixRank, 1200 x 1200          eps
 *     65  RandomWalk1 M (L = 90)           eps
 *     65  RandomWalk1 H (L = 90)           eps
 *     65  RandomWalk1 C (L = 90)         5.1e-13
 *     65  RandomWalk1 R (L = 90)         1.6e-11
 *     65  RandomWalk1 J (L = 90)         3.7e-13
 *     66  RandomWalk1 H (L = 90)          5.0e-4
 *     67  RandomWalk1 M (L = 1000)        2.8e-4
 *     67  RandomWalk1 C (L = 1000)       3.0e-12
 *     67  RandomWalk1 J (L = 1000)        7.7e-4
 *     67  RandomWalk1 H (L = 1000)         eps
 *     67  RandomWalk1 R (L = 1000)       1.9e-14
 *     71  LinearComp, r = 0              1 - eps1
 *     72  LinearComp, r = 29             1 - eps1
 *     86  HammingIndep, L = 30             eps
 *     87  HammingIndep, L = 300            eps
 *     89  HammingIndep, L = 1200           eps
 *     ----------------------------------------------
 *     All other tests were passed
 *
 * References:
 * 
 * - Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 *
 * In scientific publications which used this software, a reference to it
 * would be appreciated.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief XSH PRNG state
 */
typedef struct {
    uint64_t x;
} XSHState;


static uint64_t get_bits(void *state)
{
    XSHState *obj = state;
    obj->x ^= (obj->x << 13);
    obj->x ^= (obj->x >> 17);
    obj->x ^= (obj->x << 43);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    XSHState *obj = intf->malloc(sizeof(XSHState));
    obj->x = intf->get_seed64() | 0x1; // Seed mustn't be 0
    return (void *) obj;
}


MAKE_UINT64_PRNG("XSH", NULL)
