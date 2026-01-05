/**
 * @file xorwow.c
 * @brief xorwow pseudorandom number generator.
 * @details Fails `bspace8_8d`, `linearcomp_low` and `matrixrank` tests.
 *
 * Failures in the `express` battery: `bspace4_8d`, `bspace4_8d_dec`
 *
 * 
 *
 * BigCrush failures:
 *
 *           Test                          p-value
 *     ----------------------------------------------
 *      7  CollisionOver, t = 7            2.3e-5
 *     27  SimpPoker, r = 27              4.0e-14
 *     81  LinearComp, r = 29             1 - eps1
 *     ----------------------------------------------
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
    uint32_t d; ///< "Weyl sequence" counter
} XorWowState;


static inline uint64_t get_bits_raw(XorWowState *obj)
{
    const uint32_t d_inc = 362437;
    const uint32_t t = (obj->x ^ (obj->x >> 2));
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v << 4)) ^ (t ^ (t << 1));
    const uint32_t ans = (obj->d += d_inc) + obj->v;
    return ans;
}

static void *create(const CallerAPI *intf)
{
    XorWowState *obj = intf->malloc(sizeof(XorWowState));
    seed64_to_2x32(intf, &obj->x, &obj->y);
    seed64_to_2x32(intf, &obj->z, &obj->w);
    seed64_to_2x32(intf, &obj->v, &obj->d);
    if (obj->v == 0) {
        obj->v = 0x12345678;
    }
    return obj;
}

MAKE_UINT32_PRNG("xorwow", NULL)
