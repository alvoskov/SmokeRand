/**
 * @file icg128_p2.c
 * @brief Inversive congruential generator with power of 2 modulus.
 * @details This algorithm is much faster than 63-bit ICG with prime modulus.
 * But is still may be slower than hardware AES-128 or SIMD ChaCha12.
 * Has a period around 2^127.
 *
 * Fails the `bspace4_8d_dec` tests in SmokeRand batteries and TMFn tests
 * in PractRand 0.94-0.96.
 *
 * References:
 *
 * 1. Eichenauer-Herrmann J. Inversive Congruential Pseudorandom Numbers:
 *    A Tutorial // International Statistical Review. 1992. V. 60. N 2.
 *    P. 167-176. https://doi.org/10.2307/1403647
 * 2. Lemire D. Computing the inverse of odd integers
 *    https://lemire.me/blog/2017/09/18/computing-the-inverse-of-odd-integers/
 * 3. Hurchalla J. An Improved Integer Modular Multiplicative Inverse
 *    (modulo 2^w). 2022. https://arxiv.org/pdf/2204.04342
 * 3. https://arxiv.org/pdf/1209.6626v2
 *
 * Python code for generating reference values:
 *
 *     x = 12345
 *     for i in range(0, 10000):
 *         x = (18000_69069_69069_69069 * pow(x, -1, 2**128) + 1234513250) % 2**128
 *     print(hex(x))
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x_lo;
    uint64_t x_hi;
} Icg128State;

static inline uint64_t f64(uint64_t x, uint64_t y)
{
    return y * (2 - y * x); 
}

static uint64_t modinv64_p2(uint64_t x)
{
    uint64_t y = (3 * x) ^ 2; // 5 bits
    y = f64(x, y); // 10 bits
    y = f64(x, y); // 20 bits
    y = f64(x, y); // 40 bits
    y = f64(x, y); // 80 bits
    return y;
}

/**
 * @brief Calculates modular multiplicative inverse for the given 128-bit
 * \f$a\f$ and \f$m = 2^{128}\f$, i.e. \f$ ax \mod 2^{128} = 1 \f$.
 * @details The algorithm was initially suggested by Albert Chan in the
 * comments of D. Lemire blog for computation of modular multiplicative inverse
 * for 64-bit numbers for old 32-bit processors. The idea is very simple:
 *
 * \f[
 * \begin{cases}
 * a = & a_1 b + a_0 \\
 * x = & x_1 b + x_1 \\
 * \end{cases}
 * \f]
 *
 * where \f$ b=2^{64} \f$ for the 128-bit case. Then our equation can be written
 * the next way:
 *
 * \f[
 * a_1 x_1 b^2 + (a_0 x_1 + a_1 x_0) b + a_0 x_0 \mod b^2 = 1 \Rightarrow
 * \left( a_0 x_1 + a_1 x_0 + H_b(a_0 x_0)\right) b + L_b(a_0 x_0) \mod b^2 = 1
 * \f]
 *
 * Where L and H are functions that take the lower and the higher parts of the
 * 128-bit product. It leads to the next equations system:
 *
 * \f[
 * \begin{cases}
 * a_0 x_0 \mod b & = 1 \\
 * a_0 x_1 + \underbrace{a_1 x_0 + H_b(a_0 x_0)}_{=T} \mod b & = 0 \\
 * \end{cases}
 * \Rightarrow
 * \begin{cases}
 * x_0 = a_0^{-1} \mod b \\
 * x_1 + x_0 T \mod b = 0 \\
 * \end{cases}
 * \f]
 */
static void modinv128_p2(Icg128State *obj) 
{
    const uint64_t a0 = obj->x_lo;       // a = (a1 << 64) | a0
    const uint64_t x0 = modinv64_p2(a0); // a0 x0 = 1
    uint64_t terms = obj->x_hi * x0;     // term a1 x0
    {
        uint64_t prod_hi;
        (void) unsigned_mul128(a0, x0, &prod_hi); // term a0 x0 / 2^64
        terms += prod_hi;
    }
    const uint64_t x1 = x0 * -terms; // a0 x1 + terms = 0
    obj->x_hi = x1;                  // x = a^-1 mod 2^128
    obj->x_lo = x0;
}

/**
 * @brief Performs one ICG iteration, returns the upper 64 bits.
 * @details The next formula is used:
 *
 * \f[
 * x_{n+1} = \left{a x_n^{-1}\mod 2^{128} + b\right} \mod 2^{128}
 * \f]
 *
 * a = 18000690696906969069, b = 1234513250.
 *
 * Note: the \f$ a \mod 4 = 1 \f$, \f$ b \mod 4 = 2 \f$ conditions are
 * satisfied, so the generator has the maximal period length \f$2^{127}\f$.
 * The multiplier is fairly good for 128-bit LCGs with \f$m=2^{128}\f$.
 */
static inline uint64_t get_bits_raw(Icg128State *obj)
{
    static const uint64_t a = 18000690696906969069ull;
    uint64_t mul0_hi;
    modinv128_p2(obj);
    obj->x_lo = unsigned_mul128(a, obj->x_lo, &mul0_hi);
    obj->x_hi = a * obj->x_hi + mul0_hi;
    unsigned_add128(&obj->x_hi, &obj->x_lo, 1234513250ull);
    return obj->x_hi;
}

static void *create(const CallerAPI *intf)
{
    Icg128State *obj = intf->malloc(sizeof(Icg128State));
    obj->x_lo = intf->get_seed64() | 0x1;
    obj->x_hi = intf->get_seed64();
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    Icg128State obj = {.x_lo = 12345, .x_hi = 0};
    uint64_t u, u_ref = 0xee3a0b4abe6b0bc4;
    for (int i = 0; i < 10000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("ICG128_P2", run_self_test)
