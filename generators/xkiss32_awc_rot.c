/**
 * @file xkiss32_awc_rot.c
 * @brief A modification of KISS algorithm (2007 version) by J. Marsaglia
 * with parameters tuned by A.L. Voskov to get rid of integers overflows
 * that may be useful for porting to Modula-2 and Oberon.
 * @details It doesn't use multiplication: it is a combination or xoroshiro64,
 * and AWC (add with carry) generator. Doesn't require 64-bit integers.
 * Actually this is PRNG is an experimental attempt to make high quality PRNG
 * for Oberon-07 with the next restrictions:
 *
 * - No integer overflows during addition and multiplication.
 * - Only ROR, LSH and bitwise XOR are allowed inside LFSR part.
 * - Only 32-bit integers are allowed.
 *
 * This generator passes SmokeRand `full` battery, TestU01 BigCrush and
 * PractRand 0.94 at least up to 16 TiB.
 *
 * References:
 *
 * 1. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 2. https://groups.google.com/g/comp.lang.fortran/c/5Bi8cFoYwPE
 * 3. https://talkchess.com/viewtopic.php?t=38313&start=10
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
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
 * @brief XKISS32/AWC/ROT pseudorandom number generator state.
 */
typedef struct {
    uint32_t x;
    uint32_t awc_x0;
    uint32_t awc_x1;
    uint32_t awc_c;
} Xkiss32AwcRotState;


static inline uint64_t get_bits_raw(Xkiss32AwcRotState *obj)
{
    // LFSR part
    obj->x ^= obj->x << 1;
    obj->x ^= rotl32(obj->x, 9) ^ rotl32(obj->x, 27);
    // AWC part
    uint32_t t = obj->awc_x0 + obj->awc_x1 + obj->awc_c;
    obj->awc_x1 = obj->awc_x0;
    obj->awc_c  = t >> 26;
    obj->awc_x0 = t & 0x3ffffff;
    // Output function
    uint32_t u = (obj->awc_x0 << 6) ^ (obj->awc_x1 * 29u);
    return obj->x ^ u;
}


static void *create(const CallerAPI *intf)
{
    Xkiss32AwcRotState *obj = intf->malloc(sizeof(Xkiss32AwcRotState));
    obj->x = intf->get_seed32();
    if (obj->x == 0) {
        obj->x = 0xDEADBEEF;
    }
    uint64_t seed = intf->get_seed64();
    obj->awc_x0 = (seed >> 32) & 0x3ffffff;
    obj->awc_x1 = seed & 0x3ffffff;
    obj->awc_c  = (obj->awc_x0 == 0 && obj->awc_x1 == 0) ? 1 : 0;
    return obj;
}


/**
 * @brief Test values were obtained from the code itself.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref = 0x453EFE6E;
    uint32_t u;
    Xkiss32AwcRotState obj = {
        .x  = 12345678,
        .awc_x0 = 3, .awc_x1 = 2, .awc_c = 1};
    for (long i = 0; i < 1000000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%lX; reference: 0x%lX\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("XKISS32/AWC/ROT", run_self_test)
