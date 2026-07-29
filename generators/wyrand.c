/**
 * @file wyrand.c
 * @brief wyrand pseudorandom number generator. Passes BigCrush and PractRand
 * batteries of statistical tests. Requires 128-bit integers.
 * @details The original wyrand (v41, v42, v43) was developed by Wang Yi
 * (https://github.com/wangyi-fudan/), the `wyranda_par` modification was
 * designed by Reiner Pope (https://github.com/reinerp).
 *
 * References:
 *
 * - Wang Yi. wyhash project, public domain (Unlicense).
 *   https://github.com/wangyi-fudan/wyhash/blob/master/wyhash.h
 * - testingRNG, wyrand.h file by D. Lemire (Apache 2.0 license)
 *   https://github.com/lemire/testingRNG/blob/master/source/wyrand.h
 * - https://github.com/wangyi-fudan/wyhash/issues/156
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} WyRandState;

#define WYRAND_TEMPLATE(name, inc, xor) \
static inline uint64_t get_bits_##name##_raw(WyRandState *obj) \
{ \
    uint64_t hi, lo; \
    obj->x += (inc); \
    lo = unsigned_mul128(obj->x, obj->x ^ (xor), &hi); \
    return lo ^ hi; \
}

WYRAND_TEMPLATE(v41, 0xa0761d6478bd642fULL, 0xe7037ed1a0b428dbULL)
MAKE_GET_BITS_WRAPPERS(v41)

// https://github.com/wangyi-fudan/wyhash/commit/d7e33b3d8ebeb1884ce4cb6d0484d6f7b55d5c12
WYRAND_TEMPLATE(v42, 0xc3f0c6b4964e6c17ULL, 0xe80f8b3a95b44d35ULL)
MAKE_GET_BITS_WRAPPERS(v42)

// https://github.com/wangyi-fudan/wyhash/commit/b3b56570898eee5a433f6bea9d5d8e4b0732027f
WYRAND_TEMPLATE(v43, 0x2d358dccaa6c78a5ULL, 0x8bb84b93962eacc9ULL)
MAKE_GET_BITS_WRAPPERS(v43)

// https://github.com/hoxxep/rapidrand/pull/1/changes/adf9447169664d244307f7aff31574578170573e
// https://github.com/wangyi-fudan/wyhash/issues/156
static inline uint64_t get_bits_wyranda_par_raw(WyRandState *obj)
{
    const uint64_t x_old = obj->x;
    uint64_t hi, lo;    
    obj->x += 0x2d358dccaa6c78a5ULL;
    lo = unsigned_mul128(obj->x, x_old ^ 0x8bb84b93962eacc9ULL, &hi);
    return lo ^ hi;
}

MAKE_GET_BITS_WRAPPERS(wyranda_par)



static void *create(const CallerAPI *intf)
{
    WyRandState *obj = intf->malloc(sizeof(WyRandState));
    obj->x = intf->get_seed64();
    return obj;
}

/**
 * @brief An internal self-test for wyrand
 * @details The next code was used for generation of test values:
 *    x, u = 12345, 0
 *    for i in range(0, 100000):
 *        x_old = x
 *        x = (x + 0x2d358dccaa6c78a5) % 2**64
 *        mul = x * (x_old ^ 0x8bb84b93962eacc9)
 *        u = (mul ^ (mul >> 64)) % 2**64
 *    print(hex(u))
 */
static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    // The old mixer test
    {
        uint64_t u, u_ref = 0x1019967471850C04;
        WyRandState obj = {.x = 0xDEADBEEF01234567};
        for (int i = 0; i < 100000; i++) {
            u = get_bits_v41_raw(&obj);
        }
        intf->printf("wyrand:v41:  Output: %llX; reference: %llX\n",
            (unsigned long long) u, (unsigned long long) u_ref);
        is_ok = is_ok && (u == u_ref);
    }
    // The new mixer test
    {
        uint64_t u, u_ref = 0x22211416D82DC326;
        WyRandState obj = {.x = 12345};
        for (int i = 0; i < 100000; i++) {
            u = get_bits_wyranda_par_raw(&obj);
        }
        intf->printf("wyranda_par: Output: %llX; reference: %llX\n",
            (unsigned long long) u, (unsigned long long) u_ref);
        is_ok = is_ok && (u == u_ref);
    }
    return is_ok ? 1 : 0;
}


static const GeneratorParamVariant gen_list[] = {
    {"",          "wyrand:v43",       64, default_create, get_bits_v43, get_sum_v43},
    {"a_par",     "wyrand:a_par",     64, default_create, get_bits_wyranda_par, get_sum_wyranda_par},
    {"v43",       "wyrand:v43",       64, default_create, get_bits_v43, get_sum_v43},
    {"v42",       "wyrand:v42",       64, default_create, get_bits_v42, get_sum_v42},
    {"v41",       "wyrand:v41",       64, default_create, get_bits_v41, get_sum_v41},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"wyrand is based on a non-bijective scrambler for 64-bit discrete Weyl\n"
"sequence. Theoretically it is a non-uniform generator (not all 2^64 values\n"
"can be generated) but an empirical quality is good\n"
"The next param values are supported:\n"
"  a_par - a new version that should pass the 64-bit collision test\n"
"  v43 - from wyhash_final_version_4_3 (default)\n"
"  v42 - from wyhash_final_version_4_2\n"
"  v41 - from wyhash_final_version_4_1\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
