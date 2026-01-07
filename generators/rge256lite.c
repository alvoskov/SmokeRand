/**
 * @file rge256lite.c
 * @brief RGE256 is a nonlinear generator based on ARX nonlinear transformation.
 * @details The RGE256-lite generator was suggested by by Steven Reid; there are
 * several versions of RGE256 algorithm (with the same name!), this
 * implementation is based on the JavaScript code of the "lite" version with
 * 3 rounds.
 *
 * Passes SmokeRand `express`, `brief`, `default`, `full` batteries, TestU01
 * SmallCrush, Crush and BigCrush batteries, PractRand 0.94 >= 1 TiB.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
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
 * Reentrant C version for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RGE256lite chaotic PRNG state.
 */
typedef struct {
    uint32_t s[8];
    int nrounds;
} RGE256LiteState;


static inline uint32_t RGE256LiteState_next(RGE256LiteState *obj)
{
    uint32_t *s = obj->s;    
    for (int i = 0; i < obj->nrounds; i++) {
        // Quad updates
        s[0] += s[1]; s[1] = rotl32(s[1] ^ s[0], 7);
        s[2] += s[3]; s[3] = rotl32(s[3] ^ s[2], 9);
        s[4] += s[5]; s[5] = rotl32(s[5] ^ s[4], 13);
        s[6] += s[7]; s[7] = rotl32(s[7] ^ s[6], 18);
        // Cross coupling
        s[0] ^= s[4];
        s[1] ^= s[5];
        s[2] ^= s[6];
        s[3] ^= s[7];
    }
    return s[0] ^ s[4];
}


static inline uint64_t get_bits_raw(void *state)
{
    return RGE256LiteState_next(state);
}


static void *create(const CallerAPI *intf)
{
    RGE256LiteState *obj = intf->malloc(sizeof(RGE256LiteState));
    // Seeding
    seeds_to_array_u32(intf, obj->s, 7);
    obj->s[7] = 0x243F6A88; // To prevent bad states
    obj->nrounds = 3;
    // Warmup
    for (int i = 0; i < 10; i++) {
        (void) RGE256LiteState_next(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("RGE256lite", NULL)
