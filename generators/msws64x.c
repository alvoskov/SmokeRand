/**
 * @file msws64x.c
 * @brief A modification of "Middle-Square Weyl Sequence PRNG" that
 * uses 128-bit multiplication and XOR instead of rotations.
 * @details Passes `express`, `brief`, `default` and `full` batteries.
 *
 * References:
 *
 * 1. Bernard Widynski Middle-Square Weyl Sequence RNG
 *    https://arxiv.org/abs/1704.00358
 *
 * @copyright MSWS algorithm is developed by Bernard Widynski.
 *
 * MSWS64X modification and its implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Middle-square Weyl sequence PRNG state.
 */
typedef struct {
    uint64_t x; ///< Buffer for output function/
    uint64_t w; ///< "Weyl sequence" counter.
} MswsState;


static inline uint64_t get_bits_raw(void *state)
{
    static const uint64_t s = 0x9e3779b97f4a7c15;
    MswsState *obj = state;
    uint64_t sq_hi;
    obj->x += (obj->w += s);
    obj->x = unsigned_mul128(obj->x, obj->x, &sq_hi); // |32bit|32bit||32bit|32bit||
    obj->x ^= sq_hi; // Middle squares (64 bits) + XORing
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    MswsState *obj = intf->malloc(sizeof(MswsState));
    obj->x = intf->get_seed64();
    obj->w = intf->get_seed64();
    return (void *) obj;
}


MAKE_UINT64_PRNG("Msws64x", NULL)
