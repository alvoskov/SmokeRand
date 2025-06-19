/**
 * @file xkiss64_awc.c
 * @brief XKISS64/AWC is a 64-bit modificaiton of 32-bit KISS algorithm
 * (2007 version) by G. Marsaglia with parameters tuned by A.L. Voskov.
 * The initial 32-bit version with slightly altered parameters is known
 * as JKISS32 (see the paper by David Jones [1]).
 * @details It doesn't use multiplication: it is a combination or xorshift64,
 * discrete Weyl sequence and AWC (add with carry) generator.
 *
 * Note: AWC (add-with-carry) part is based on the next prime:
 *
 * \f[
 * m = (2^{55})^2 + (2^{55})^1 - 1
 * \f]
 *
 * So we use slightly more complex output function than the original KISS.
 * KISS64/AWC is probably one of the fastest modifications of KISS.
 *
 * References:
 *
 * 1. George Marsaglia. Fortran and C: United with a KISS. 2007.
 *    https://groups.google.com/g/comp.lang.fortran/c/5Bi8cFoYwPE
 * 2. George Marsaglia, Arif Zaman. A New Class of Random Number Generators //
 *    Ann. Appl. Probab. 1991. V. 1. N.3. P. 462-480
 *    https://doi.org/10.1214/aoap/1177005878
 * 3. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 4. https://talkchess.com/viewtopic.php?t=38313&start=10
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define K64_AWC_MASK 0x7fffffffffffff
#define K64_AWC_SH 55
#define K64_WEYL_INC 0x9E3779B97F4A7C15

/**
 * @brief KISS64/AWC PRNG state.
 */
typedef struct {
    uint64_t weyl; ///< Discrete Weyl sequence state.
    uint64_t xsh; ///< xorshift64 state.
    uint64_t awc_x0; ///< AWC state, \f$ x_{n-1}) \f$.
    uint64_t awc_x1; ///< AWC state, \f$ x_{n-2}) \f$.
    uint64_t awc_c; ///< AWC state, carry.
} Kiss64AwcState;


static inline uint64_t get_bits_raw(void *state)
{
    Kiss64AwcState *obj = state;
    // xorshift64 part
    obj->xsh ^= obj->xsh << 13;
    obj->xsh ^= obj->xsh >> 17;
    obj->xsh ^= obj->xsh << 43;
    // AWC (add with carry) part
    uint64_t t = obj->awc_x0 + obj->awc_x1 + obj->awc_c;
    obj->awc_x1 = obj->awc_x0;
    obj->awc_c  = t >> K64_AWC_SH;
    obj->awc_x0 = t & K64_AWC_MASK;
    // Discrete Weyl sequence part
    obj->weyl += K64_WEYL_INC;
    // Combined output
    return ((obj->awc_x0 << 9) ^ obj->awc_x1) + obj->xsh + obj->weyl;
}


static void *create(const CallerAPI *intf)
{
    Kiss64AwcState *obj = intf->malloc(sizeof(Kiss64AwcState));
    obj->awc_x0 = intf->get_seed64() & K64_AWC_MASK;
    obj->awc_x1 = intf->get_seed64() & K64_AWC_MASK;
    obj->awc_c  = (obj->awc_x0 == 0 && obj->awc_x1 == 0) ? 1 : 0;
    obj->xsh    = intf->get_seed64();
    if (obj->xsh == 0) {
        obj->xsh = 0x123456789ABCDEF;
    }
    obj->weyl   = intf->get_seed64();
    return (void *) obj;
}

/**
 * @brief Test values were obtained from Python code.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref = 0x3d5898fbd8636929;
    uint64_t u;
    Kiss64AwcState obj = {
        .weyl = 12345678, .xsh  = 87654321,
        .awc_x0 = 3, .awc_x1 = 2, .awc_c = 1};
    for (int i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("XKISS64/AWC", run_self_test)
