/**
 * @file rge256ex.c
 * @brief RGE256ex is an improved modification of RGE256 nonlinear generator.
 * @details It is a modification of RGE256 generator suggested by Steven Reid.
 * The author of the modification is Alexey L. Voskov:
 *
 * - A linear part with 64-bit counter was added (so the minimal period
 *   is at least 2^64).
 * - Extra rotations were added to the ARX nonlinear transformation that allowed
 *   to reduce the number of rounds and get rid of the output function.
 *
 * S. Reid suggested several different version of RGE256 algorithm, this variant
 * is based on its simplified version.
 *
 * Passes SmokeRand `express`, `brief`, `default`, `full` batteries, TestU01
 * SmallCrush, Crush and BigCrush batteries, PractRand 0.94 >= 1 TiB.
 *
 * References:
 *
 * 1. Reid, S. (2025). RGE-256: A New ARX-Based Pseudorandom Number Generator
 *    With Structured Entropy and Empirical Validation. Zenodo.
 *    https://doi.org/10.5281/zenodo.17713219
 * 2. https://rrg314.github.io/RGE-256-Lite/
 *
 * @copyright The original RGE256 algorithm was suggested by Steven Reid.
 *
 * Reengineering to RGE256ex and reentrant C version for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RGE256ex PRNG state.
 */
typedef struct {
    uint32_t s[8]; ///< Chaotic (nonlinear) part.
    uint64_t ctr;  ///< Linear (discrete Weyl sequence) part.
    int pos;       ///< Position in the output buffer (chaotic part).
} RGE256ExState;


static void RGE256ExState_next(RGE256ExState *obj)
{
    uint32_t *s = obj->s;    
    const uint64_t ctr = obj->ctr;
    obj->s[0] += (uint32_t) ctr;
    obj->s[1] += (uint32_t) (ctr >> 32);
    obj->ctr += 0x9E3779B97F4A7C15;
    for (int i = 0; i < 2; i++) {
        s[0] += s[1]; s[1] ^= s[0];
        s[2] += s[3]; s[3] ^= rotl32(s[2], 6);
        s[4] += s[5]; s[5] ^= rotl32(s[4], 12);    
        s[6] += s[7]; s[7] ^= rotl32(s[6], 18);

        s[5] ^= s[0]; s[0] += rotl32(s[5], 7);
        s[6] ^= s[1]; s[1] += rotl32(s[6], 11);
        s[7] ^= s[2]; s[2] += rotl32(s[7], 13);
        s[4] ^= s[3]; s[3] += rotl32(s[4], 17);
    }
}



static inline uint64_t get_bits_raw(RGE256ExState *obj)
{
    if (obj->pos >= 8) {
        RGE256ExState_next(obj);
        obj->pos = 0;
    }
    uint32_t out = obj->s[obj->pos++];
    return out;
}

static void *create(const CallerAPI *intf)
{
    RGE256ExState *obj = intf->malloc(sizeof(RGE256ExState));
    seeds_to_array_u32(intf, obj->s, 8);
    obj->pos = 8;
    obj->ctr = intf->get_seed32();
    // Warmup
    for (int i = 0; i < 10; i++) {
        (void) RGE256ExState_next(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("RGE256ex", NULL)
