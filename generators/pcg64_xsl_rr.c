/**
 * @file pcg64_xsl_rr.c
 * @brief PCG64 PRNG implementation with PCG-XSL-RR output function.
 * @details PCG (permuted congruental generators) is a family of pseudorandom
 * number generators invented by M.E. O'Neill. This version has 128-bit state
 * and 64-bit output and its period is \f$2^{128}\f$. It passes all batteries
 * from SmokeRand test suite.
 *
 * This variant of PCG is commonly used in numerical libraries such as NumPy
 * and SciPy. Its only drawback is dependence on 128-bit integer arithmetics
 * that is outside of C99 standard and relies on non-portable extensions.
 *
 * PCG64 with RCG-XSL-RR output function also passes SmallCrush, Crush and
 * BigCrush batteries from TestU01.
 * 
 * @copyright The PCG algorithm family is suggested by M.E. O'Neill
 * (https://pcg-random.org).
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
    Lcg128State *obj = state;
    // Just ordinary 128-bit LCG
    (void) Lcg128State_a128_iter(state, 0x2360ED051FC65DA4, 0x4385DF649FCCF645, 1);    
    // Output XSL RR function
//#ifdef UINT128_ENABLED
//    uint64_t x_lo = (uint64_t) obj->x, x_hi = obj->x >> 64;
//#else
    uint64_t x_lo = obj->x_low, x_hi = obj->x_high;
//#endif
    unsigned int rot = x_hi >> 58; // 64 - 6
    uint64_t xsl = x_hi ^ x_lo;
    return rotr64(xsl, rot);
}


static void *create(const CallerAPI *intf)
{
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));
    Lcg128State_seed(obj, intf);
    return (void *) obj;
}

/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
//#ifdef UINT128_ENABLED
//    Lcg128State obj = {.x = 1234567890};
//#else
    Lcg128State obj = { .x_low = 1234567890, .x_high = 0 };
//#endif
    uint64_t u, u_ref = 0x8DE320BB801095E2;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("Lcg128Xsl64", run_self_test)
