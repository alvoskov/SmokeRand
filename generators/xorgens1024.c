/**
 * @file xorgens1024.c
 * @brief xorgens1024 is a xorshift-like PRNG developed by R.P. Brent.
 * Its period \f$ is 2^{1024} - 1. \f$
 * @details The next parameteres are used in this file:
 *
 *    n 1024 w 64 r 16  s 7  a 34 b 29 c 25 d 31  Wt 439 delta 25
 *
 * NOTE: this implementation is intentionally suboptimal (may be around
 * 1.5-2 cpb) and uses the entire state update instead of the circle buffer.
 * It makes it suitable for automated period deduction using the `lfsr` battery.
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
 * @brief Xorgens1024 PRNG state
 */
typedef struct {
    uint64_t x[16];
} Xorgens1024State;


static inline uint64_t get_bits_raw(Xorgens1024State *obj)
{
    uint64_t a = obj->x[0] ^ (obj->x[0] << 34); a ^= a >> 29; // shifts (a,b)
    uint64_t b = obj->x[9] ^ (obj->x[9] << 25); b ^= b >> 31; // shifts (c,d)
    for (int i = 0; i < 15; i++) {
        obj->x[i] = obj->x[i + 1];
    }
    obj->x[15] = a ^ b;
    return obj->x[15];
}


static void *create(const CallerAPI *intf)
{
    Xorgens1024State *obj = intf->malloc(sizeof(Xorgens1024State));
    seeds_to_array_u64(intf, obj->x, 16);
    if (obj->x[15] == 0) { // State mustn't be all zeros
        obj->x[15] = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorgens1024", NULL)
