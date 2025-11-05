/**
 * @file icg64.c
 * @brief Inversive congruential generator with prime modulus.
 * @details This generator is very slow, much slower than DES, 3DES,
 * Magma and Kuznyechik. Has a period around 2^63.
 *
 * The algorithm for modular inversion is taken from [2] (see algorithm 2.20).
 * A similar algorithm is used in TestU01 sources.
 *
 * References:
 *
 * 1. Eichenauer-Herrmann J. Inversive Congruential Pseudorandom Numbers:
 *    A Tutorial // International Statistical Review. 1992. V. 60. N 2.
 *    P. 167-176. https://doi.org/10.2307/1403647
 * 2. Hankerson D., Menezes A., Vanstone S. Guide to elliptic curve
 *    cryptography. 2004. Springer-Verlag New York Inc.
 *    https://doi.org/10.1007/b97644
 *
 * Python code for generating reference values:
 *
 *     x = 12345
 *     for i in range(0, 10000):
 *         x = (pow(x, -1, 2**63 - 25) + 1) % (2**63 - 25)
 *     print(hex(x * 2))
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static const int64_t ICG64_MOD = 0x7fffffffffffffe7; // 2^63 - 25

typedef struct {
    int64_t x;
} Icg64State;

/**
 * @brief Calculates \f$ f^{-1} \mod p \f$ using Algorithm 2.20 from
 * Hankenson et al. [2].
 */
int64_t modinv64(int64_t p, int64_t a)
{
    int64_t u = a, v = p, x1 = 1, x2 = 0;
    if (a == 0) return 0;
    while (u != 1) {
        int64_t q = v / u;
        int64_t r = v - q * u;
        int64_t x = x2 - q * x1;
        v = u; u = r; x2 = x1; x1 = x;
    }
    if (x1 < 0)
        x1 += p;
    return x1;
}


static inline uint64_t get_bits_raw(void *state)
{
    Icg64State *obj = state;
    obj->x = (modinv64(ICG64_MOD, obj->x) + 1) % ICG64_MOD;
    return (uint64_t) obj->x >> 31;
}

static void *create(const CallerAPI *intf)
{
    Icg64State *obj = intf->malloc(sizeof(Icg64State));
    obj->x = (int64_t) (intf->get_seed64() % (uint64_t) ICG64_MOD);
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    Icg64State obj = {.x = 12345};
    uint32_t u, u_ref = 0xe5a6beea;
    for (int i = 0; i < 10000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("ICG64", run_self_test)
