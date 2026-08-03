/**
 * @file xorshift192.c
 * @brief xorshift192 is a 64-bit modification of the classical xorshift
 * generator suggested by G. Marsaglia.
 * @details The shifts triples for xorshift192 were found by Yura Sokolov.
 * The good triple was selected by A.L. Voskov by means of SmokeRand tests,
 * especially `hamming_distr` and `bspace` tests.
 *
 * References:
 *
 * 1. https://github.com/funny-falcon/xorshift256and192
 * 2. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * Notes about triples:
 *
 * - Probably good triple: [39 23 3]
 * - Probably acceptable triple: [51 1 3], [19 33 5], [49 3 13]
 * - Not so bad triples (less problems with birthday spacings
 *   in brief battery, but bad HW test): [37 9 42], [36 23 32], [16 19 45],
 *   [30 29 8], [31 25 36]
 * - Bad triples (fail birthday spacings and/or gap test):
 *   [11 3 17], [11 1 11], [11 3 14], [13 1 50], [5 19 7], [11 43 2], [13 11 1]
 *
 * @copyright The xorshift algorithm family was suggested by G. Marsaglia,
 * the xorshift192 modification was designed by Yura Sokolov. Tuning and
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
 * @brief Xorshift192 PRNG state
 */
typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
} Xorshift192State;


static inline uint64_t get_bits_raw(Xorshift192State *obj)
{
    uint64_t t = obj->x ^ (obj->x << 29); // a
    t ^= t >> 23; // b
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = (obj->z ^ (obj->z >> 3)) ^ t; // c
    return obj->z;
}


static void *create(const CallerAPI *intf)
{
    Xorshift192State *obj = intf->malloc(sizeof(Xorshift192State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    if (obj->z == 0) { // State mustn't be all zeros
        obj->z = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorshift192", NULL)
