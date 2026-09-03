/**
 * @file xorrot512.c
 * @brief A xorshift-style LFSR that has the \f$2^{512} - 1\f$ period.
 * @details It uses the next step matrix:
 *
 *                                  | I 0 0 0  0 0 I A |
 *     |x0 x1 x2 x3 x4 x5 x6 x7 | * | I 0 0 0  0 0 0 0 |
 *                                  | 0 I 0 0  0 0 0 0 |
 *                                  | 0 0 I 0  0 0 0 0 |
 *                                  | 0 0 0 I  0 0 0 0 |
 *                                  | 0 0 0 0  I 0 0 0 |
 *                                  | 0 0 0 0  0 I 0 0 |
 *                                  | 0 0 0 0  0 0 I B |
 *
 * Poly degrees:
 *
 *    [7 9 42]: 233  [7 26 41]: 255 [9 1 24]: 235  [9 8 17]: 261
 *    [ 9 25 60]:237 [11  4 13]:245 [11 35 40]:277 [13  1 30]:245
 *    [13 26 33]:253 [15 19 49]:235 [15 25 55]:247 [15 27 46]:255
 *    [19 21 59]:263 [21 47 60]:239 [23  6 13]:247 [23 32 39]:243
 *    [27 13 16]:    [29  3 50]:    [29 23 52]:    [31  3 61]:
 *    [31 47 58]:    [33  2  9]:    [33 24 63]:    [35 24 47]:
 *    [35 25 28]:    [37 29 36]:    [39  7 52]:    [39 33 48]:
 *    [41  9 50]:    [41 15 59]:    [43 10 27]:    [45 10 61]:
 *    [49 10 57]:    [51  1 14]:259 [51  4 55]:    [51  8 25]:
 *    [51 27 56]:    [51 31 38]:    [51 47 56]:    [53  9 60]:
 *    [55 29 34]:247 [57 14 21]:231 [57 29 32]:223 [57 34 41]:243
 *    [59 16 29]:255 [59 28 49]:223 [61 14 25]:267 [63 22 57]:263
 *    [63 54 57]:263
 *
 * Notes about quality:
 *
 * 1. [7 26 41] - slightly suspicious colloveer20_2d in full (p>0.5 systematically?)
 * 2. [15 27 46] - full ok, >= 8 TiB in PractRand except BRank
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
    uint64_t x[8];
} Xorrot512State;


static inline uint64_t get_bits_raw(Xorrot512State *obj)
{
    static const int a = 15, b = 27, c = 46; // full ok, >= 8 TiB in PractRand (except BRank)
    const uint64_t x0 = obj->x[0], x7 = obj->x[7];
    obj->x[0] = x0 ^ obj->x[1];
    obj->x[1] = obj->x[2];
    obj->x[2] = obj->x[3];
    obj->x[3] = obj->x[4];
    obj->x[4] = obj->x[5];
    obj->x[5] = obj->x[6];
    obj->x[6] = x0 ^ x7;
    obj->x[7] = (x0 << a) ^ obj->x[6] ^ rotl64(x7, b) ^ rotl64(x7, c);
    return x0;
}

static void *create(const CallerAPI *intf)
{
    Xorrot512State *obj = intf->malloc(sizeof(Xorrot512State));
    for (int i = 0; i < 8; i++) {
        obj->x[i] = intf->get_seed64();
    }
    if (obj->x[0] == 0) {
        obj->x[0] = 0x123456789ABCDEF;
    }
    return obj;
}


MAKE_UINT64_PRNG("xorrot512", NULL)
