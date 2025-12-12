/**
 * @file komirand.c
 * @brief Komirand generator is a nonlinear chaotic pseudorandom number
 * generator suggested by Aleksey Vaneev. The algorithm description and
 * official test vectors can be found at https://github.com/avaneev/komihash
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * @copyright The komirand algorithm was developed by Aleksey Vannev.
 *
 * An initial wrapper for SmokeRand: (C) 2025 Scott Baker
 *
 * Refactoring for eliminating dependence from `komihash.h`, adding
 * internal self-tests and warmup addition:
 * 
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief Komirand PRNG state.
 */
typedef struct {
    uint64_t st1;
    uint64_t st2;
} KomirandState;

static inline uint64_t get_bits_raw(KomirandState *state)
{
    static const uint64_t inc = 0xaaaaaaaaaaaaaaaa;
    uint64_t s1 = state->st1, s2 = state->st2, mul_hi;
    const uint64_t mul_lo = unsigned_mul128(s1, s2, &mul_hi);
    s2 += mul_hi + inc;
    s1 = mul_lo ^ s2;
    state->st1 = s1;
    state->st2 = s2;
    return s1;
}

static void *create(const CallerAPI *intf)
{
    KomirandState *obj = intf->malloc(sizeof(KomirandState));
    obj->st1 = intf->get_seed64();
    obj->st2 = obj->st1;
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

/**
 * @brief An internal self-test based on 
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[] = { // Official test vectors
        0xaaaaaaaaaaaaaaaa, 0xfffffffffffffffe,
        0x4924924924924910, 0xbaebaebaebaeba00,
        0x400c62cc4727496b, 0x35a969173e8f925b,
        0xdb47f6bae9a247ad, 0x98e0f6cece6711fe
    };
    KomirandState obj = {0, 0};
    int is_ok = 1;
    for (int i = 0; i < 8; i++) {
        const uint64_t u = get_bits_raw(&obj);
        intf->printf("Out: %16.16" PRIX64 "; ref: %16.16" PRIX64 "\n",
            u, u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("Komirand", run_self_test)
