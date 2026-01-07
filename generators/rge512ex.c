/**
 * @file rge512ex.c
 * @brief RGE512ex is an improved modification of RGE256 nonlinear generator.
 * @details It is a modification of RGE256 generator suggested by Steven Reid.
 * The author of the modification is Alexey L. Voskov:
 *
 * - 32-bit words were replaced into 64-bit words. New rotations were selected
 *   empirically but without a real opitimization (just intuitive selection and
 *   running SmokeRand batteries for versions with 1-2 rounds)
 * - A linear part with 64-bit counter was added (so the minimal period
 *   is at least 2^64).
 * - Extra rotations were added to the ARX nonlinear transformation that allowed
 *   to reduce the number of rounds and get rid of the output function.
 *
 * S. Reid suggested several different version of RGE256 algorithm, this variant
 * is based on its simplified version.
 *
 * Passes SmokeRand `express`, `brief`, `default`, `full` batteries.
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
 * Reengineering to RGE512ex and reentrant C version for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RGE512ex PRNG state.
 */
typedef struct {
    uint64_t s[8]; ///< Chaotic (nonlinear) part.
    uint64_t ctr;  ///< Linear (discrete Weyl sequence) part.
    int pos;
} RGE512ExState;


static void RGE512ExState_next(RGE512ExState *obj)
{
    uint64_t *s = obj->s;    
    const uint64_t ctr = obj->ctr;
    obj->s[0] += ctr;
    obj->ctr += 0x9E3779B97F4A7C15;
    for (int i = 0; i < 2; i++) {
        s[0] += s[1]; s[1] ^= rotl64(s[0], 3);
        s[2] += s[3]; s[3] ^= rotl64(s[2], 12);
        s[4] += s[5]; s[5] ^= rotl64(s[4], 24);    
        s[6] += s[7]; s[7] ^= rotl64(s[6], 48);

        s[5] ^= s[0]; s[0] += rotl64(s[5], 7);
        s[6] ^= s[1]; s[1] += rotl64(s[6], 17);
        s[7] ^= s[2]; s[2] += rotl64(s[7], 23);
        s[4] ^= s[3]; s[3] += rotl64(s[4], 51);
    }
}

static void RGE512ExState_init(RGE512ExState *obj, const uint64_t *seed)
{
    for (size_t i = 0; i < 8; i++) {
        obj->s[i] = seed[i];
    }
    obj->ctr = obj->s[7];
    obj->pos = 0;
    // Warmup
    for (int i = 0; i < 10; i++) {
        RGE512ExState_next(obj);
    }
}



static inline uint64_t get_bits_raw(void *state)
{
    RGE512ExState *obj = state;
    if (obj->pos >= 8) {
        RGE512ExState_next(obj);
        obj->pos = 0;
    }
    uint64_t out = obj->s[obj->pos++];
    return out;
}

static void *create(const CallerAPI *intf)
{
    uint64_t seed[8];
    RGE512ExState *obj = intf->malloc(sizeof(RGE512ExState));
    seeds_to_array_u64(intf, seed, 8);
    RGE512ExState_init(obj, seed);
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t seed[8] = {
        0x1122, 0x2233, 0x3344, 0x4455, 0x5566, 0x6677, 0x7788, 0x8899
    };
    static const uint64_t ref[16] = {
        0xC073BCE8B96814F1, 0x4B2295BC57FD36DA, 0x3B6F52399FD95ECD, 0x2C08FE9E8C3F8B4F,
        0xB4279FA1EC271392, 0x9ABDEB8BBBC8FB53, 0xCF92F18B8C7A2528, 0x19BE95A8BD3BD26E,
        0xCB430C151019C5C5, 0x023061E25D5191F9, 0xB57E2B94AAFC2A56, 0x383936D1E447284C,
        0x6C9FCD33D43F0618, 0x02C56431D463603C, 0x79522458141BDC6E, 0x92C968A92DF88735
    };
    intf->printf("Diffusion demo\n");
    RGE512ExState *obj = intf->malloc(sizeof(RGE512ExState));
    for (int i = 0; i < 8; i++) {
        obj->s[i] = 0;
    }
    obj->pos = 8;
    obj->ctr = 1;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            const uint64_t u = get_bits_raw(obj);
            intf->printf("0x%16.16llX", (unsigned long long) u);
        }
        intf->printf("\n");
    }

    intf->printf("An internal self-test\n");
    RGE512ExState_init(obj, seed);
    for (int i = 0; i < 134; i++) {
        (void) get_bits_raw(obj);
    }

    int is_ok = 1;
    for (int i = 0; i < 16; i++) {
        const uint64_t u = get_bits_raw(obj), u_ref = ref[i];
        intf->printf("%16.16llX %16.16llX\n",
            (unsigned long long) u, (unsigned long long) u_ref);
        if (u != u_ref) {
            is_ok = 0;
        }
    }

    intf->free(obj);
    return is_ok;
}

MAKE_UINT64_PRNG("RGE512ex", run_self_test)
