/**
 * @file swbmwc64.c
 * @brief A combined 64-bit generator made from subtract-with-borrow (SWB) and
 * multiply-with-carry PRNG.
 * @details This generator is designed for 64-bit computers and is based on
 * the next recurrent formula:
 *
 * \f[
 * x_{n} = x_{n-s} - x_{n-r} - b_{n-1} \mod 2^{64}
 * \f]
 *
 * \f[
 * b_{n} = \begin{cases}
 *   0 & \textrm{ if } x_{n-s} - x_{n-r} - b_{n-1} \ge 0 \\
 *   1 & \textrm{ if } x_{n-s} - x_{n-r} - b_{n-1} <   0 \\
 * \end{cases}
 * \f]
 *
 * \f[
 * y_{n} = ay_{n-1} + c_{n-1} \mod 2^{32}
 * \f]
 *
 * \f[
 * c_{n} = \dfrac{ay_{n-1} + c_{n-1}}{2^{32}}
 * \f]
 *
 * The \f$ r = 13 \f$ and \f$ s = 7 \f$ lags are selected by A.L. Voskov
 * to provide the \f$ m = 2^{64*13} - 2^{64*7} + 1 \f$ prime modulus. The
 * MWC multiplier \f$ a = 2^{32} - 10001272 \f$ was also selected by
 * A.L. Voskov. The modulus m is Proth prime. The period of the SWB64 generator
 * is about \f$ 10^{246} \f$ or \f$ 2^{818} \f$ and can be estimated
 * by the next Python 3.x program:
 *
 *    import sympy, math
 *    b = 2**64
 *    m = b**13 - b**7 + 1
 *    print("Prime test from sympy: ", sympy.isprime(m))
 *    a = 23
 *    print("Proth test: ", pow(a, (m - 1) // 2, m) - (-1 % m) == 0)
 *    print("Finding generator period...")
 *    o = sympy.n_order(b, m) # or sympy.n_order(pow(b, -1, m), m)
 *    print("Period: ", o)
 *    print("log10(per): ", math.log10(o), "; log2(per)", math.log2(o))
 *
 * Although the used SWB and MWC generators themselves have a low quality,
 * its combination passes SmokeRand tests batteries. It has better quality
 * than the original `swbmwc32` from DIEHARD test suite.
 *
 * References:
 *
 * 1. George Marsaglia, Arif Zaman. A New Class of Random Number Generators //
 *    Ann. Appl. Probab. 1991. V. 1. N.3. P. 462-480
 *    https://doi.org/10.1214/aoap/1177005878
 *
 * @copyright The SWB algorithm was suggested by G.Marsaglia and A.Zaman.
 *
 * Tuning for 64-bit output and reentrant implementation for SmokeRand:
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_A 13
#define SWB_B 7

/**
 * @brief 64-bit SWBMWC state.
 */
typedef struct {    
    uint64_t x[SWB_A];
    uint64_t c;
    uint64_t mwc;
    int i;
    int j;
} SwbMwc64State;


static inline uint64_t get_bits_raw(void *state)
{
    static const uint64_t MWC_A = 0xff676488; // 2^32 - 10001272
    SwbMwc64State *obj = state;
    // SWB part
    const uint64_t xj = obj->x[obj->j], xi = obj->x[obj->i];
    const uint64_t t = xj - xi - obj->c;
    obj->c = (xj < t) ? 1 : 0;
    obj->x[obj->i] = t;
    if (obj->i == 0) { obj->i = SWB_A; }
	if (obj->j == 0) { obj->j = SWB_A; }
    obj->i--;
    obj->j--;
    // MWC part
    obj->mwc = (obj->mwc & 0xFFFFFFFFu) * MWC_A + (obj->mwc >> 32);
    return t + obj->mwc;
}


static void *create(const CallerAPI *intf)
{
    SwbMwc64State *obj = intf->malloc(sizeof(SwbMwc64State));    
    for (size_t i = 0; i < SWB_A; i++) {
        obj->x[i] = intf->get_seed64();
    }
    obj->c = 1;
    obj->x[1] |= 1;
    obj->x[2] = (obj->x[2] >> 1) << 1;
    obj->mwc = (intf->get_seed64() >> 8) | 0x1;
    obj->i = SWB_A - 1; obj->j = SWB_B - 1;
    return obj;
}


MAKE_UINT64_PRNG("SWBMWC64", NULL)
