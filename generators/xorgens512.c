/**
 * @file xorgens512.c
 * @brief xorgens512 is a xorshift-like PRNG developed by R.P. Brent.
 * Its period \f$ is 2^{512} - 1. \f$
 * @details The next parameteres are used in this file:
 *
 *    r = 8, s = 1, a = 37, b = 26, c = 29, d = 34
 *
 * NOTE: this implementation is intentionally suboptimal and uses the
 * entire state update instead of the circle buffer. It makes it suitable
 * for automated period deduction using the `lfsr` battery.
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
 * @brief Xorgens512 PRNG state
 */
typedef struct {
    uint64_t x[8];
} Xorgens512State;

static inline uint64_t get_bits_raw(Xorgens512State *obj)
{
    uint64_t a = obj->x[0] ^ (obj->x[0] << 37); a ^= a >> 26; // shifts (a,b)
    uint64_t b = obj->x[7] ^ (obj->x[7] << 29); b ^= b >> 34; // shifts (c,d)
    obj->x[0] = obj->x[1];
    obj->x[1] = obj->x[2];
    obj->x[2] = obj->x[3];
    obj->x[3] = obj->x[4];
    obj->x[4] = obj->x[5];
    obj->x[5] = obj->x[6];
    obj->x[6] = obj->x[7];
    obj->x[7] = a ^ b;
    return obj->x[7];
}


static void *create(const CallerAPI *intf)
{
    Xorgens512State *obj = intf->malloc(sizeof(Xorgens512State));
    seeds_to_array_u64(intf, obj->x, 8);
    if (obj->x[7] == 0) { // State mustn't be all zeros
        obj->x[7] = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorgens512", NULL)
