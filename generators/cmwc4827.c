/**
 * @file cmwc4827.c
 * @brief A complementary multiply-with-carry generator MWC4827 by G.Marsaglia.
 * @details It is a CMWC (complementary-multiply-with-carry) generator with
 * the \f$ p = 4095 b^{4827} + 1 \f$ prime modulus. The a = 4095 value makes
 * possible an efficient implementation of multiplications by means of bit
 * masks and shifts.
 *
 * However due to a specific multiplier this generator fails the `gap16_count0`
 * test in `brief`, `default` and `full` SmokeRand batteries. It passes
 * BigCrush but fails PractRand 0.94
 *
 *
 * Comments of G. Marsaglia about the period of the generator:
 *
 *     After about 5 days, one of them found p=4095*b^4827+1. And it turned out
 *     that the order of b mod p was the maximum possible, k=(p-1)/2^6, (since
 *     we "lose" one 2 because 2 cannot be primitive, and must lose at least
 *     five more because b=2^5 and k is 4095*2^154458). Thus b^k mod p = 1, and
 *     for each prime divisor q of k, q=2,3,5,7,13: b^(k/q) mod p is not 1.
 *
 *     Unlike p=ab^r-1, a prime of the form p=ab^r+1 leads to CMWC
 *     (Complimentary-Multiply-With-Carry) RNGs, in which we again imagine
 *     forming t=a*x+c in 64 bits and again seek c as the top 32 bits, but
 *     rather than x=(t mod b) for MWC, the new x is x=(b-1)-(t mod b),
 *     that is x=~(t mod b), using C's ~ op.
 *
 *     Here is a C version of the resulting CMWC method for p=4095*b^4827+1,
 *     using only 32-bit arithmetic, with verified period 4095*2^154458, faster
 *     and simpler than the previously posted MWC4691() and readily adapted
 *     for either signed or unsigned integers and for Fortran or other
 *     programming languages.
 *     
 * References:
 *
 * 1. https://www.thecodingforums.com/threads/the-cmwc4827-rng-an-improvement-on-mwc4691.736178/
 *
 * @copyright The CMWC4827 algorithm was developed by G. Marsaglia.
 *
 * Adaptation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief CMWC4827 PRNG state.
 */
typedef struct {
    uint32_t x[4827]; ///< Generated values
    uint32_t c; ///< Carry
    int pos; ///< Current position in the buffer
} Cmwc4827State;


static void Cmwc4827State_init(Cmwc4827State *obj, uint32_t xcng, uint32_t xs)
{
    if (xs == 0) {
        xs = 0x12345678;
    }
    for (size_t i = 0; i < 4827; i++) {
        xcng = 69069u * xcng + 13579u;
        xs ^= (xs << 13);
        xs ^= (xs >> 17);
        xs ^= (xs << 5);
        obj->x[i] = xcng + xs;
    }
    obj->c = 0;
    obj->pos = 4827;
    obj->c = 1271;
}

static inline uint64_t get_bits_raw(Cmwc4827State *obj)
{
    obj->pos = (obj->pos < 4826) ? obj->pos + 1 : 0;
    const uint32_t x = obj->x[obj->pos];
    const uint32_t t = (x << 12) + obj->c;
    obj->c = (x >> 20) - (t < x);
    return obj->x[obj->pos] = ~(t - x);
}

static void *create(const CallerAPI *intf)
{
    Cmwc4827State *obj = intf->malloc(sizeof(Cmwc4827State));
    uint32_t seed_lo, seed_hi;
    seed64_to_2x32(intf, &seed_lo, &seed_hi);
    Cmwc4827State_init(obj, seed_hi, seed_lo);
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    uint32_t x, x_ref = 1346668762;
    Cmwc4827State *obj = intf->malloc(sizeof(Cmwc4827State));
    Cmwc4827State_init(obj, 123456789, 362436069); 
    for (unsigned long i = 0; i < 1000000000; i++) {
        x = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("x = %22u; x_ref = %22u\n", x, x_ref);
    intf->free(obj);
    return x == x_ref;
}


MAKE_UINT32_PRNG("Cmwc4827", run_self_test)
