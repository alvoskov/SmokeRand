/**
 * @file gmwc128.c
 * @brief GMWC128 - Goresky-Klapper generalized multiply-with-carry
 * generator.
 * @details Description by S.Vigna:
 *
 * This is a Goresky-Klapper generalized multiply-with-carry generator
 * (see their paper "Efficient Multiply-with-Carry Random Number
 * Generators with Maximal Period", ACM Trans. Model. Comput. Simul.,
 * 13(4), p. 310-321, 2003) with period approximately 2^127. While in
 * general slower than a scrambled linear generator, it is an excellent
 * generator based on congruential arithmetic.
 *
 * As all MWC generators, it simulates a multiplicative LCG with prime
 * modulus m = 0xff002aae7d81a646007d084a4d80885f and multiplier given by
 * the inverse of 2^64 modulo m. Note that the major difference with
 * standard (C)MWC generators is that the modulus has a more general form.
 * This additional freedom in the choice of the modulus fixes some of the
 * theoretical issues of (C)MWC generators, at the price of one 128-bit
 * multiplication, one 64-bit multiplication, and one 128-bit sum.
 *
 * Comments by A.L. Voskov. The next Python 3.8+ code allows to make
 * a connection between constants in the code:
 *
 *     minus_a0 = 0x7d084a4d80885f
 *     a1 = 0xff002aae7d81a646
 *     a0 = (2**64 - minus_a0)
 *     print("a0 = ", hex(a0))
 *     a0inv = pow(a0, -1, 2**64)
 *     print("a0inv = ", hex(a0inv))
 *     m = a1*2**64 - a0
 *     print("m = ", hex(m))
 *
 * The output is in a perfect agreement with constants suggested by Vigna.
 *
 *     a0 =  0xff82f7b5b27f77a1
 *     a0inv =  0x9b1eea3792a42c61
 *     m = 0xff002aae7d81a645007d084a4d80885f
 *
 * @copyright The implementation is based on public domain code by
 * S.Vigna (vigna@acm.org).
 *
 * Portable subroutines for 128-bit multiplication, 128-bit addition
 * and adaptation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief GMWC128 state.
 * @details The state must be initialized so that GMWC_MINUS_A0 <= c <= GMWC_A1.
 * For simplicity, we suggest to set c = 1 and x to a 64-bit seed.
 *
 */
typedef struct {
    uint64_t x;
    uint64_t c;
} GMWC128State;

#define GMWC_MINUSA0 0x7d084a4d80885fU
#define GMWC_A0INV   0x9b1eea3792a42c61U
#define GMWC_A1      0xff002aae7d81a646U


static inline uint64_t get_bits_raw(GMWC128State *obj)
{
    uint64_t t_lo, t_hi;
    // 1) const __uint128_t t = GMWC_A1 * (__uint128_t) obj->x + obj->c;
    t_lo = unsigned_muladd128(GMWC_A1, obj->x, obj->c, &t_hi);
    // 2) obj->x = GMWC_A0INV * (uint64_t)t;
    obj->x = GMWC_A0INV * t_lo;
    // 3) obj->c = (t + GMWC_MINUSA0 * (__uint128_t)obj->x) >> 64;
    (void) unsigned_muladd128(GMWC_MINUSA0, obj->x, t_lo, &obj->c);
    obj->c += t_hi;
//    c_hi += t_hi;
//    obj->c = c_hi;
	return obj->x;
}

static void *create(const CallerAPI *intf)
{
    GMWC128State *obj = intf->malloc(sizeof(GMWC128State));
    obj->x = intf->get_seed64();
    obj->c = 1;
    return obj;
}

int run_self_test(const CallerAPI *intf)
{
    GMWC128State obj = {.x = 0x123456789ABCDEF, .c = 1};
    const uint64_t u_ref = 0x33D56C3F38C7E6C7;
    uint64_t u;
    for (int i = 0; i < 1000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Output: %llX, reference: %llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("GMWC128", run_self_test)
