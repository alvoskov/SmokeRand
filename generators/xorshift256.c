/**
 * @file xorshift256.c
 * @brief xorshift256 is a 64-bit modification of the classical xorshift
 * generator suggested by G. Marsaglia.
 * @details The shifts triples for xorshift256 were found by Yura Sokolov.
 * The good triple was selected by A.L. Voskov by means of SmokeRand tests,
 * especially `hamming_distr` and `bspace` tests.
 *
 * References:
 *
 * 1. https://github.com/funny-falcon/xorshift256and192
 * 2. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * @copyright The xorshift algorithm family was suggested by G. Marsaglia,
 * the xorshift256 modification was designed by Yura Sokolov. Tuning and
 * adaptation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift256 PRNG state
 */
typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
    uint64_t w;
} Xorshift256State;


static inline uint64_t get_bits_raw(Xorshift256State *obj)
{
    uint64_t t = obj->x ^ (obj->x << 25); // a
    t ^= t >> 1; // b
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = (obj->w ^ (obj->w >> 36)) ^ t; // c
    return obj->z;
}


static void *create(const CallerAPI *intf)
{
    Xorshift256State *obj = intf->malloc(sizeof(Xorshift256State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    if (obj->w == 0) { // State mustn't be all zeros
        obj->w = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorshift256", NULL)
