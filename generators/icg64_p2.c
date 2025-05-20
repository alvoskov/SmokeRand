/**
 * @file icg64_p2.c
 * @brief Inversive congruential generator with power of 2 modulus
 * @details This algorithm is much faster than 63-bit ICG with prime modulus.
 * But is still slower than hardware AES-128 or SIMD ChaCha12. Has a period
 * around 2^63.
 *
 * Fails the `bspace8_8d`, `bspace4_8d_dec`, `bspace4_16d` tests.
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
 *         x = (6906969069 * pow(x, -1, 2**64) + 1234513250) % 2**64
 *     print(hex(x))
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} Icg64State;

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

static inline uint64_t get_bits_raw(void *state)
{
    Icg64State *obj = state;
    obj->x = 6906969069ull * modinv64_p2(obj->x) + 1234513250ull;
    return obj->x >> 32;
}

static void *create(const CallerAPI *intf)
{
    Icg64State *obj = intf->malloc(sizeof(Icg64State));
    obj->x = intf->get_seed64() | 0x1;
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    Icg64State obj = {.x = 12345};
    uint32_t u, u_ref = 0x7def6e56;
    for (int i = 0; i < 10000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT32_PRNG("ICG64_P2", run_self_test)
