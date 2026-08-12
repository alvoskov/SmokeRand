/**
 * @file xorgens256.c
 * @brief xorgens256 is a xorshift-like PRNG developed by R.P. Brent.
 * Its period \f$ is 2^{256} - 1. \f$
 * @details The next parameteres are used in this file:
 *
 *     r = 4, s = 3, a = 37, b = 27, c = 29, d = 33
 *
 * References:
 *
 * - Brent R.P. Some long-period random number generators using shifts and xors
 *   // ANZIAM J. 2007. V.48. P. C188--C202. https://doi.org/10.21914/anziamj.v48i0.40
 *   Proceedings of the 13th Biennial Computational Techniques and Applications
 *   Conference, CTAC-2006. Editors: Wayne Read  and A. J. Roberts
 *
 * @copyright The xorgens algorithm family was suggested by R.P. Brent.
 *
 * Reentrant C99 implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorgens PRNG state
 */
typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
    uint64_t w;
} Xorgens256State;


static inline uint64_t get_bits_raw(Xorgens256State *obj)
{
    uint64_t a = obj->x ^ (obj->x << 37); a ^= a >> 27; // shifts (a,b)
    uint64_t b = obj->y ^ (obj->y << 29); b ^= b >> 33; // shifts (c,d)
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = a ^ b;
    return obj->w;
}


static void *create(const CallerAPI *intf)
{
    Xorgens256State *obj = intf->malloc(sizeof(Xorgens256State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    if (obj->w == 0) { // State mustn't be all zeros
        obj->w = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorgens256", NULL)
