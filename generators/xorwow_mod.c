/**
 * @file xorwow.c
 * @brief xorwow pseudorandom number generator.
 * @details 
 *
 * References:
 *
 * 1. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *    V. 8. N 14. P. 1-6. https://doi.org/10.18637/jss.v008.i14
 * 2. cuRAND library programming guide.
 *    https://docs.nvidia.com/cuda/curand/testing.html
 *
 * @copyright xorwow algorithm is developed by G.Marsaglia.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief xorwow PRNG state.
 */
typedef struct {
    uint32_t x; ///< Xorshift register
    uint32_t y; ///< Xorshift register
    uint32_t z; ///< Xorshift register
    uint32_t w; ///< Xorshift register
    uint32_t v; ///< Xorshift register
    uint32_t d1; ///< "Weyl sequence" counter
    uint32_t d2; ///< "Weyl sequence" counter
} XorWowState;


static inline uint64_t get_bits_raw(XorWowState *obj)
{
    uint32_t t = obj->x ^ (obj->x << 19); // a
    t ^= t >> 3; // b
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v >> 11)) ^ t; // c

    const uint32_t d1 = obj->d1;
    obj->d1 = d1 + obj->d2;
    obj->d2 = d1;

//    obj->d += 0x9E3779B9;
    const uint32_t ans = rotl32(d1, 1) + obj->v;
    return ans;
}

static void *create(const CallerAPI *intf)
{
    XorWowState *obj = intf->malloc(sizeof(XorWowState));
    seed64_to_2x32(intf, &obj->x, &obj->y);
    seed64_to_2x32(intf, &obj->z, &obj->w);
    seed64_to_2x32(intf, &obj->v, &obj->d1);
    if (obj->v == 0) {
        obj->v = 0x12345678;
    }
    obj->d2 = 12345678;
    return obj;
}

MAKE_UINT32_PRNG("xorwow", NULL)
