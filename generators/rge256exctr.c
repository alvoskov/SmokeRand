/**
 * @file rge256exctr.c
 * @brief RGE256ex-ctr is an counter based generator inspired by the RGE256
 * nonlinear generator.
 * @details This counter based generator was developed by Alexey L. Voskov.
 * It is based on reengineered ARX nonlinear transformations from RGE256
 * generator suggested by Steven Reid. The rounds are identical to rounds
 * in the RGE256ex generator. Even 5 rounds are enough to pass `express`,
 * `brief`, `default` and `full` SmokeRand batteries, so 6 rounds are used
 * for robustness.
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
 * @brief RGE256ex-ctr counter-based PRNG-state.
 */
typedef struct {
    uint32_t ctr[8]; ///< Counter (input block)
    uint32_t out[8]; ///< Output buffer
    int pos;         ///< Position inside the output buffer
} RGE256ExCtrState;


static inline void RGE256ExCtrState_block(RGE256ExCtrState *obj)
{
    uint32_t *s = obj->out;
    for (int i = 0; i < 8; i++) {
        obj->out[i] = obj->ctr[i];
    }
    for (int i = 0; i < 6; i++) { // Even 5 rounds is enough for `full`
        s[0] += s[1]; s[1] ^= s[0];
        s[2] += s[3]; s[3] ^= rotl32(s[2], 6);
        s[4] += s[5]; s[5] ^= rotl32(s[4], 12);    
        s[6] += s[7]; s[7] ^= rotl32(s[6], 18);

        s[5] ^= s[0]; s[0] += rotl32(s[5], 7);
        s[6] ^= s[1]; s[1] += rotl32(s[6], 11);
        s[7] ^= s[2]; s[2] += rotl32(s[7], 13);
        s[4] ^= s[3]; s[3] += rotl32(s[4], 17);
    }
    for (int i = 0; i < 8; i++) {
        obj->out[i] += obj->ctr[i];
    }
}


static void RGE256ExCtrState_init(RGE256ExCtrState *obj, const uint32_t *seed)
{
    obj->ctr[0] = 0;          obj->ctr[1] = 0;
    obj->ctr[2] = 0x243F6A88; obj->ctr[3] = 0x85A308D3;
    obj->ctr[4] = seed[0];    obj->ctr[5] = seed[1];
    obj->ctr[6] = seed[2];    obj->ctr[7] = seed[3];
    RGE256ExCtrState_block(obj);
    obj->pos = 0;
}

static inline uint64_t get_bits_raw(RGE256ExCtrState *obj)
{
    if (obj->pos >= 8) {
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        RGE256ExCtrState_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    RGE256ExCtrState *obj = intf->malloc(sizeof(RGE256ExCtrState));
    uint32_t seed[4];
    seeds_to_array_u32(intf, seed, 4);
    RGE256ExCtrState_init(obj, seed);
    return obj;
}

MAKE_UINT32_PRNG("RGE256ex-ctr", NULL)
