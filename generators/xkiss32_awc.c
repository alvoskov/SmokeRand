/**
 * @file xkiss32_awc.c
 * @brief A modification of KISS algorithm (2007 version) by J. Marsaglia
 * with parameters tuned by A.L. Voskov.
 * @details It doesn't use multiplication: it is a combination or xoroshiro64,
 * and AWC (add with carry) generator. Doesn't require 64-bit integers.
 *
 * References:
 *
 * 1. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 2. https://groups.google.com/g/comp.lang.fortran/c/5Bi8cFoYwPE
 * 3. https://talkchess.com/viewtopic.php?t=38313&start=10
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * The KISS algorithm is developed by George Marsaglia, its JKISS version
 * was suggested by David Jones.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xkiss32AwcState PRNG state.
 */
typedef struct {
    uint32_t s[2];
    uint32_t awc_x0;
    uint32_t awc_x1;
    uint32_t awc_c;
} Xkiss32AwcState;


static inline uint64_t get_bits_raw(void *state)
{
    Xkiss32AwcState *obj = state;
    // xoroshiro64 part
    uint32_t s0 = obj->s[0], s1 = obj->s[1];
    s1 ^= s0;
    s0 = rotl32(s0, 26) ^ s1 ^ (s1 << 9); // a, b
    s1 = rotl32(s1, 13); // c
    obj->s[0] = s0; obj->s[1] = s1;
    // AWC (add with carry) part
    uint32_t t = obj->awc_x0 + obj->awc_x1 + obj->awc_c;
    obj->awc_x1 = obj->awc_x0;
    obj->awc_c  = t >> 26;
    obj->awc_x0 = t & 0x3ffffff;
    // Combined output
    return s0 + s1 + ((obj->awc_x0 << 6) ^ obj->awc_x1);
}


static void *create(const CallerAPI *intf)
{
    Xkiss32AwcState *obj = intf->malloc(sizeof(Xkiss32AwcState));
    uint64_t seed_xs = intf->get_seed64();
    uint64_t seed = intf->get_seed64();
    obj->s[0] = (uint32_t) (seed_xs & 0xFFFFFFFF);
    obj->s[1] = (uint32_t) ((seed_xs >> 32) | 0x1);
    obj->awc_x0 = (uint32_t) ((seed >> 32) & 0x3ffffff);
    obj->awc_x1 = (uint32_t) (seed & 0x3ffffff);
    obj->awc_c  = (obj->awc_x0 == 0 && obj->awc_x1 == 0) ? 1 : 0;
    return (void *) obj;
}

/**
 * @brief Test values were obtained from the code itself.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref = 0x59AA1AB5;
    uint32_t u;
    Xkiss32AwcState obj = {
        .s  = {8765, 4321},
        .awc_x0 = 3, .awc_x1 = 2, .awc_c = 1};
    for (int i = 0; i < 10000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%X; reference: 0x%X\n",
        (unsigned int) u, (unsigned int) u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("Xkiss32AwcState", run_self_test)
