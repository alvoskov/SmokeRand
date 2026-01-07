/**
 * @file sqxor.c
 * @brief PRNG inspired by the Von Neumann middle squares method and
 * its modification by B.Widynski. Requires 128-bit integers.
 * @details Passes SmallCrush, Crush and BigCrush batteries
 * ("Weyl sequence" variant).
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief SQXOR 64-bit PRNG state.
 */
typedef struct {
    uint64_t w; ///< "Weyl sequence" counter state
} SqXorState;


static inline uint64_t get_bits_raw(SqXorState *obj)
{
    const uint64_t s = 0x9E3779B97F4A7C15;
    const uint64_t ww = obj->w += s; // "Weyl sequence" variant
    uint64_t sq_hi;
    // Round 1
    uint64_t x = unsigned_mul128(ww, ww, &sq_hi); // |32bit|32bit||32bit|32bit||
    x ^= sq_hi; // Middle squares (64 bits) + XORing
    // Round 2
    //sq = ((__int128) x) * ww; // Slower but a little more reliable
    x = unsigned_mul128(x, x, &sq_hi);
    x ^= sq_hi; // Middle squares (64 bits) + XORing
    // Return the result
    return x;
}

/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
    SqXorState obj = {.w = 1234567890};
    uint64_t u, u_ref = 0xB74C88775DF514;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


static void *create(const CallerAPI *intf)
{
    SqXorState *obj = intf->malloc(sizeof(SqXorState));
    obj->w = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("SqXor", run_self_test)
