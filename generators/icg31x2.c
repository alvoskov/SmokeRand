/**
 * @file icg31x2.c
 * @brief A combination of two 31-bit inversive congruential generators with
 * prime modulus.
 * @details This algorithm is slightly faster than 63-bit ICG with prime modulus
 * but is still much slower than AES-128 or ChaCha12. Has a period
 * around 2^62. Fails the `sumcollector` test from the `full` battery.
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
 *      x, y = 12345, 67890
 *      for i in range(0, 10000):
 *          x = (pow(x, -1, 2**31 - 1) + 1) % (2**31 - 1)
 *          y = (pow(y, -1, 2**31 - 19) + 1) % (2**31 - 19)
 *      u = x ^ (y * 2)
 *      print(hex(u))
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

enum {
    ICG32_MOD1 = 0x7FFFFFFF, ///< \f$ 2^{31} - 1 \f$
    ICG32_MOD2 = 0x7FFFFFED  ///< \f$ 2^{31} - 19 \f$
};


PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x1;
    uint32_t x2;
} Icg31x2State;

/**
 * @brief Calculates \f$ f^{-1} \mod p \f$ using Algorithm 2.20 from
 * Hankenson et al. [2].
 */
int32_t modinv32(int32_t p, int32_t a)
{
    int32_t u = a, v = p, x1 = 1, x2 = 0;
    if (a == 0) return 0;
    while (u != 1) {
        int32_t q = v / u;
        int32_t r = v - q * u;
        int32_t x = x2 - q * x1;
        v = u; u = r; x2 = x1; x1 = x;
    }
    if (x1 < 0)
        x1 += p;
    return x1;
}

static inline uint64_t get_bits_raw(void *state)
{
    Icg31x2State *obj = state;
    obj->x1 = (modinv32(ICG32_MOD1, obj->x1) + 1) % ICG32_MOD1;
    obj->x2 = (modinv32(ICG32_MOD2, obj->x2) + 1) % ICG32_MOD2;
    return obj->x1 ^ (obj->x2 << 1);
}

static void *create(const CallerAPI *intf)
{
    Icg31x2State *obj = intf->malloc(sizeof(Icg31x2State));
    obj->x1 = intf->get_seed32() % ICG32_MOD1;
    obj->x2 = intf->get_seed32() % ICG32_MOD2;
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    Icg31x2State obj = {.x1 = 12345, .x2 = 67890};
    uint32_t u, u_ref = 0x5742A591;
    for (int i = 0; i < 10000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT32_PRNG("ICG31x2", run_self_test)
