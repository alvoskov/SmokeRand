/**
 * @file romumono.c
 * @brief Implementation of RomuMono PRNG developed by Mark A. Overton.
 * @details It is a chaotic generator based on the invertible nonlinear mapping.
 * Its average period can be estimated as \f$ 2^{63}\f$.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * References:
 *
 * 1. Mark A. Overton. Romu: Fast Nonlinear Pseudo-Random Number Generators
 *    Providing High Quality. https://doi.org/10.48550/arXiv.2002.11331
 * 2. Discussion of Romu: https://news.ycombinator.com/item?id=22447848
 *
 * @copyright Mono algorithm is developed by Mark Overton.
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
 * @brief RomuMono state
 */
typedef struct {
    uint64_t x;
} RomuMonoState;


static inline uint64_t get_bits_raw(RomuMonoState *obj)
{
    const uint32_t ans = (uint32_t) obj->x;
    obj->x = rotl64(obj->x, 32) * 15241094284759029579U;
    return ans;
}


static void *create(const CallerAPI *intf)
{
    RomuMonoState *obj = intf->malloc(sizeof(RomuMonoState));
    obj->x = intf->get_seed64();
    if (obj->x == 0) {
        obj->x = 0x123456789ABCDEF;
    }
    return obj;
}


MAKE_UINT32_PRNG("RomuMono", NULL)
