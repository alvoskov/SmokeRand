/**
 * @file mrg32k3a.c
 * @brief MRG32k3a pseudorandom number generator.
 * @details It consists of two multiple recurrence generators:
 * \f[
 * \begin{cases}
 * x_{1,n} = 1403580 x_{n-2} - 810728 x_{n-3} \mathop{\mathrm{mod}} m_1 \\
 * x_{2,n} = 527612 x_{n-1} - 1370589 x_{n-3} \mathop{\mathrm{mod}} m_2 \\
 * \end{cases}
 * \f]
 * where \f$ m_1 = 2^{32} - 209 \f$ and \f$ m_2 = 2^{32} - 22853 \f$ are
 * primes sligtly less than \f$ 2^{32} \f$.
 *
 * Output is obtained from a combination of \f$ x_{1,n} \f$ and \f$ x_{2,n} \f$:
 *
 * \f[
 *   x_{n} = x_{1,n} - x_{2,n} \mathop{\mathrm{mod}} {m_1}
 * \f]
 *
 * The original implementation by P.L'Ecuyer was designed in the era of
 * 32-bit computers and uses IEEE-754 doubles to work with signed integers
 * less than \f$ 2^{53} \f$. This module uses signed 64-bit integers instead.
 *
 * References:
 *
 * 1. L'Ecuyer P. Good Parameters and Implementations for Combined Multiple
 *    Recursive Random Number Generators // Operations Research. 1999. V.47.
 *    N 1. P.159-164. https://doi.org/10.1287/opre.47.1.159
 * 2. https://www-labs.iro.umontreal.ca/~simul/rng/MRG32k3a.c
 *
 * @copyright The MRG32k3a algorithm is developed by P.L'Ecuyer.
 * 
 * Integer-based implementation for SmokeRand with internal self-tests:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MRG32k3a PRNG state.
 * @details Buffers are organized the next way:
 * \f[
 *   [x_{n-3} x_{n-2} x_{n-1}]
 * \f]
 * The seeds for s1 and s2 must be in [0, m1 - 1] and not all 0. 
 */
typedef struct {
    uint32_t s1[3]; ///< Component 1
    uint32_t s2[3]; ///< Component 2
} Mrg32k3aState;


static const int64_t m1 = 4294967087ll; ///< Prime modulus \f$ 2^{32} - 209 \f$
static const int64_t m2 = 4294944443ll; ///< Prime modulus \f$ 2^{32} - 22853 \f$

/**
 * @brief The multiplicative recursive generator.
 * @details Resembles LCG, based on the relationship:
 *
 * \f[
 *   x_n = a x_{n - r} - b x_{n - q} \mod m
 * \f]
 * where m is a prime slightly less than \f$ 2^{32} \f$.
 * 
 * @param ind_a  Used for r = 3 - ind_a
 * @param ind_b  Used for q = 3 - ind_b
 */
static inline int64_t component(uint32_t *s,
    const int64_t a, const size_t ind_a,
    const int64_t b, const size_t ind_b,
    const int64_t m)
{
    const int64_t p = (a * (int64_t) s[ind_a] - b * (int64_t) s[ind_b] + b*m) % m;
    s[0] = s[1];
    s[1] = s[2];
    s[2] = (uint32_t) p;
    return p;
}

inline uint32_t make_seed(const CallerAPI *intf, const uint64_t m)
{
    uint32_t seed;
    do {
        seed = intf->get_seed32();
    } while (seed == 0 || seed >= m);
    return seed;
}

static void *create(const CallerAPI *intf)
{
    Mrg32k3aState *obj = intf->malloc(sizeof(Mrg32k3aState));
    obj->s1[0] = make_seed(intf, (uint64_t) m1);
    obj->s1[1] = make_seed(intf, (uint64_t) m1);
    obj->s1[2] = make_seed(intf, (uint64_t) m1);
    obj->s2[0] = make_seed(intf, (uint64_t) m2);
    obj->s2[1] = make_seed(intf, (uint64_t) m2);
    obj->s2[2] = make_seed(intf, (uint64_t) m2);
    return obj;
}

/**
 * @brief Returns the 32-bit integer in the \f$ [1; m1] \f$ range.
 * @details It will systematically fail the `FPF-14+6/16:cross` test
 * from PractRand at 4 TiB sample. The failure is systematic and reproducible.
 * Renormalization is recommended before its usage.
 */
static inline uint32_t Mrg32k3aState_get_u32_raw(Mrg32k3aState *obj)
{
    static const int64_t a12  = 1403580ll, a13n = 810728ll;
    static const int64_t a21  = 527612ll,  a23n = 1370589ll;
    const int64_t p1 = component(obj->s1, a12, 1, a13n, 0, m1);
    const int64_t p2 = component(obj->s2, a21, 2, a23n, 0, m2);
    const int64_t u = (p1 <= p2) ? (p1 - p2 + m1) : (p1 - p2);
    return (uint32_t) u;
}

/**
 * @brief Renormalizes the \f$ [1; m1] \f$ interval to the
 * \f$ [0; 2^{32}-1]\f$ interval.
 * @details It uses the next formula for renormalization:
 *
 * \f[
 * x = (u - 1) \frac{2^{32} - 1}{m_1 - 1} = (u - 1) + c \frac{u - 1}{2^{55}}
 * \f]
 *
 * The `c` coefficient is an integer and bitwise shifts are used instead
 * of division.
 *
 * The code to test the conversion formula:
 *
 *    #include <stdio.h>
 *    #include <stdint.h>
 *    static const int64_t m1 = 4294967087ll;
 *    static inline uint32_t conv_int(uint32_t x) {
 *        return x + ((1753219158U * (uint64_t) x) >> 55);
 *    }
 *    int main() {
 *        for (uint32_t x = 0; x < (uint32_t) m1; x++) {
 *            if (x < 20 || x > (uint32_t) (m1 - 20)) {
 *                printf("%X %X\n",
 *                    (unsigned int) x, (unsigned int) conv_int(x));
 *            }
 *        }
 *    }
 *
 * Python 3.x code to find the coefficients:
 *
 *    from fractions import Fraction
 *    m1 = 4294967087
 *    coeff = Fraction(2**32 - 1, m1 - 1)
 *    base = 2**55
 *    ifr = int((coeff - 1) * base) + 1
 *    coeff_approx = 1 + Fraction(ifr, base)
 *    print(float(coeff_approx - coeff))
 *    print(ifr)
 *    print(ifr * 2**32 / 2**64)
 */
static inline uint64_t get_bits_raw(Mrg32k3aState *obj)
{
    const uint32_t r = Mrg32k3aState_get_u32_raw(obj) - 1;
    const uint32_t x = (uint32_t) (r + ((1753219158U * (uint64_t) r) >> 55));
    return x;
}

/**
 * @brief Internal self-test based on the values obtained
 * by running the original code by P.L'Ecuyer.
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t seed = 12345;
    Mrg32k3aState obj;
    uint32_t u_ref[] = {
        0x1C6D4BA6, 0xAEDE0194, 0x6D85B214, 0x45A88A44,
        0xA3D5BEC0, 0x583A7E3A, 0xBD2798DA, 0xD0BB36FD
    };
    int is_ok = 1;
    for (int i = 0; i < 3; i++) {
        obj.s1[i] = seed;
        obj.s2[i] = seed;
    }
    for (int i = 0; i < 10000; i++) {
        Mrg32k3aState_get_u32_raw(&obj);
    }
    intf->printf("%8s %s\n", "Output", "Reference");
    for (int i = 0; i < 8; i++) {
        const uint64_t u = Mrg32k3aState_get_u32_raw(&obj);
        intf->printf("0x%.8lX 0x%.8lX\n",
            (unsigned long) u, (unsigned long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT32_PRNG("MRG32k3a", run_self_test)
