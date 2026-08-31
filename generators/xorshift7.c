/**
 * @file xorshift7
 * @brief xorshift7 is a xorshift-like LFSR with the maximal period of
 * `2**256 - 1`. It has higher statistical quality than classical xorshift
 * espeically in Hamming weight related tests.
 * @details
 *
 * Note: this implementation intentionally excludes a circle buffer / counters
 * to make the application of the `lfsr` battery possible.
 *
 * References:
 *
 * 1. Panneton F., L'Ecuyer P. On the xorshift random number generators //
 *    // ACM Trans. Model. Comput. Simul. 2005. V. 15. N 4. P.346-361.
 *    https://doi.org/10.1145/1113316.1113319
 *
 * @copyright xorshift7 algorithm was developed by F. Panneton and P. L'Ecuyer. 
 * 
 * Reentrant C99 implementation with:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t x[8];
} Xorshift7State;


static inline uint64_t get_bits_raw(Xorshift7State *obj)
{
    uint32_t t, y, *x = obj->x;
    t = x[7]; t ^= t << 13; y = t ^ (t << 9);
    t = x[4]; y ^= t ^ (t << 7);
    t = x[3]; y ^= t ^ (t >> 3);
    t = x[1]; y ^= t ^ (t >> 10);
    t = x[0]; t ^= t >> 7; y ^= t ^ (t << 24);
    x[0] = x[1];
    x[1] = x[2];
    x[2] = x[3];
    x[3] = x[4];
    x[4] = x[5];
    x[5] = x[6];
    x[6] = x[7];
    x[7] = y;
    return y;
}

static void *create(const CallerAPI *intf)
{
    Xorshift7State *obj = intf->malloc(sizeof(Xorshift7State));
    seeds_to_array_u32(intf, obj->x, 8);
    if (obj->x[0] == 0) {
        obj->x[0] = 0x12345678;
    }
    return obj;
}

MAKE_UINT32_PRNG("xorshift7", NULL)
