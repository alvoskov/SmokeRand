/**
 * @file xkiss32sh_awc.c
 * @brief A modification of KISS algorithm (2007 version) by J. Marsaglia
 * with parameters tuned by A.L. Voskov.
 * @details It doesn't use multiplication: it is a combination of SHR3
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
 * @brief Xkiss32ShAwcState PRNG state.
 */
typedef struct {
    uint32_t xs;
    uint32_t awc_x0;
    uint32_t awc_x1;
    uint32_t awc_c;
} Xkiss32ShAwcState;


static inline uint64_t get_bits_raw(void *state)
{
    Xkiss32ShAwcState *obj = state;
    // xorshift32 part
    obj->xs ^= obj->xs << 13;
    obj->xs ^= obj->xs >> 17;
    obj->xs ^= obj->xs << 5;
    // AWC (add with carry) part
    uint32_t t = obj->awc_x0 + obj->awc_x1 + obj->awc_c;
    obj->awc_x1 = obj->awc_x0;
    obj->awc_c  = t >> 26;
    obj->awc_x0 = t & 0x3ffffff;
    // Combined output
    return obj->xs + ((obj->awc_x0 << 6) ^ obj->awc_x1);
}


static void *create(const CallerAPI *intf)
{
    Xkiss32ShAwcState *obj = intf->malloc(sizeof(Xkiss32ShAwcState));
    uint64_t seed_xs = intf->get_seed64();
    uint64_t seed = intf->get_seed64();
    obj->xs = seed_xs & 0xFFFFFFFF;
    if (obj->xs == 0) obj->xs = 0xDEADBEEF;
    obj->awc_x0 = (seed >> 32) & 0x3ffffff;
    obj->awc_x1 = seed & 0x3ffffff;
    obj->awc_c  = (obj->awc_x0 == 0 && obj->awc_x1 == 0) ? 1 : 0;
    return (void *) obj;
}

/**
 * @brief Test values were obtained from the code itself.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref = 0x9D5B6B8;
    uint32_t u;
    Xkiss32ShAwcState obj = {
        .xs  = 8765,
        .awc_x0 = 3, .awc_x1 = 2, .awc_c = 1};
    for (int i = 0; i < 10000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%X; reference: 0x%X\n",
        (unsigned int) u, (unsigned int) u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("XKISS32/SHORT/AWC", run_self_test)
