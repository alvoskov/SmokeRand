/**
 * @file xkiss16_awc.c
 * @brief XKISS16/AWC is a 16-bit modificaiton of 32-bit KISS algorithm
 * (2007 version) by G. Marsaglia with parameters tuned by A.L. Voskov.
 * @details The initial 32-bit version [5] with slightly altered parameters
 * is known as JKISS32 (see the paper by David Jones [7]). The next changes
 * were made to adapt the algorithm to 16-bit CPUs:
 *
 * 1. xorshift32 was replaced to xoroshiro32+ (the parameters are taken
 *    from [1,2]). It has too short period for the general purpose PRNG but
 *    is designed for to 16-bit register and is useful for combined PRNGs.
 * 2. AWC (add-with-carry) generator was tuned for 16-bit machines that
 *    support 16-bit addition with carry bit. It is based on the
 *    \f$ m = (2^{16})^2 + (2^{16})^1 - 1 \f$ prime and essentially an LCG
 *    with prime modulus and bad multiplier.
 * 3. Discrete Weyl sequence was made 16-bit.
 *
 * It has a period around \f$ 2^{63} \f$ that is probably too small for serious
 * simulations but is probably good enough for 16-bit PRNG designed for
 * retrocomputing.
 *
 * References:
 *
 * 1. https://forums.parallax.com/discussion/comment/1448759/#Comment_1448759
 * 2. https://github.com/ZiCog/xoroshiro/blob/master/src/main/c/xoroshiro.h
 * 3. https://groups.google.com/g/prng/c/Ll-KDIbpO8k/m/bfHK4FlUCwAJ
 * 4. https://forums.parallax.com/discussion/comment/1423789/#Comment_1423789
 * 5. George Marsaglia. Fortran and C: United with a KISS. 2007.
 *    https://groups.google.com/g/comp.lang.fortran/c/5Bi8cFoYwPE
 * 6. George Marsaglia, Arif Zaman. A New Class of Random Number Generators //
 *    Ann. Appl. Probab. 1991. V. 1. N.3. P. 462-480
 *    https://doi.org/10.1214/aoap/1177005878
 * 7. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 8. https://talkchess.com/viewtopic.php?t=38313&start=10
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define K16_AWC_MASK 0xFFFF
#define K16_AWC_SH 16
#define K16_WEYL_INC 0x9E39


/**
 * @brief XKISS16/AWC state.
 */
typedef struct {
    uint16_t weyl;
    uint16_t xs; ///< xorshift16
    uint16_t awc_x0; ///< AWC state, \f$ x_{n-1}) \f$.
    uint16_t awc_x1; ///< AWC state, \f$ x_{n-2}) \f$.
    uint16_t awc_c; ///< AWC state, carry.
} Xkiss16AwcState;


static inline uint16_t rotl16(uint16_t x, int r)
{
    return (x << r) | (x >> ((-r) & 15));
}


static inline uint16_t Xkiss16AwcState_get_bits(Xkiss16AwcState *obj)
{
    // xorshift16
    // https://gist.github.com/t-mat/8b2c183ae50480c7998f4d9ab2271b1d
    // 3,13,9 - 5/3
    // 1,15,7 - 9/3
    // 5,11,6 - 5/4
    obj->xs ^= obj->xs << 3;
    obj->xs ^= obj->xs >> 13;
    obj->xs ^= obj->xs << 9;
    // AWC (add with carry) part
    uint32_t t = obj->awc_x0 + obj->awc_x1 + obj->awc_c;
    obj->awc_x1 = obj->awc_x0;
    obj->awc_c  = t >> K16_AWC_SH;
    obj->awc_x0 = t & K16_AWC_MASK;
    obj->weyl += K16_WEYL_INC;
    // Combined output
    return (rotl16(obj->awc_x0, 3) ^ (obj->awc_x1)) + rotl16(obj->xs + obj->weyl, obj->awc_x1);
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = Xkiss16AwcState_get_bits(state);
    uint32_t lo = Xkiss16AwcState_get_bits(state);
    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Xkiss16AwcState *obj = intf->malloc(sizeof(Xkiss16AwcState));
    uint64_t seed = intf->get_seed64();
    obj->xs = (seed >> 16) & 0xFFFF;
    if (obj->xs == 0) {
        obj->xs = 0xDEAD;
    }
    obj->weyl = 0;
    obj->awc_x0 = (seed >> 32) & 0xFFFF;
    obj->awc_x1 = (seed >> 48) & 0xFFFF;
    obj->awc_c  = (obj->awc_x0 == 0 && obj->awc_x1 == 0) ? 1 : 0;
    return obj;
}

/**
 * @brief Test values were obtained from Python code.
 */
/*
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref = 0xBC84B06E;
    uint32_t u;
    Xkiss16AwcState obj = {
        .weyl = 1234, .s  = {8765, 4321},
        .awc_x0 = 3, .awc_x1 = 2, .awc_c = 1};
    for (int i = 0; i < 10000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%X; reference: 0x%X\n",
        (unsigned int) u, (unsigned int) u_ref);
    return u == u_ref;
}
*/

MAKE_UINT32_PRNG("XKISS16/AWC", NULL)
