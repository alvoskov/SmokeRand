/**
 * @file hicg64.c
 * @brief Hybrid inversive congruential generator with power of 2 modulus.
 * @details This algorithm is much faster than 63-bit ICG with prime modulus.
 * But is still slower than hardware AES-128 or SIMD ChaCha12. Has a period
 * around 2^63.
 *
 * If upper 32 bits are analysed - fails the `bspace8_8d`, `bspace4_8d_dec`,
 * `bspace4_16d` tests. If all 64 bits are analysed - fails almost everything.
 * Note: even its HIGHER bits fail the birthday spacings test with decimation
 * with `step = 4096`, i.e. from the `brief` battery.
 *
 * References:
 *
 * 1. Riera C., Roy T., Sarkar S., Pantelimon S. A Hybrid Inversive Congruential
 *    Pseudorandom Number Generator with High Period. // European Journal of Pure
 *    and Applied Mathematics. 2021. V.14 N 1. P. 1-18.
 *    https://doi.org/10.29020/nybg.ejpam.v14i1.3852
 * 2. Eichenauer-Herrmann J. Inversive Congruential Pseudorandom Numbers:
 *    A Tutorial // International Statistical Review. 1992. V. 60. N 2.
 *    P. 167-176. https://doi.org/10.2307/1403647
 * 3. Lemire D. Computing the inverse of odd integers
 *    https://lemire.me/blog/2017/09/18/computing-the-inverse-of-odd-integers/
 * 4. Hurchalla J. An Improved Integer Modular Multiplicative Inverse
 *    (modulo 2^w). 2022. https://arxiv.org/pdf/2204.04342
 * 5. https://arxiv.org/pdf/1209.6626v2
 *
 * Python code for generating reference values:
 *
 *     a, b, c = 1886906, 706715, 807782
 *     x_m1, x_m2 = 1725239, 430227
 *     for i in range(0, 10000):
 *         x_new = (a * pow(x_m1, -1, 2**64) + b * x_m2 + c) % 2**64;
 *         x_m2, x_m1 = x_m1, x_new
 *     print(hex(x_m1))
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x_m1;
    uint64_t x_m2;
} Hicg64State;

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

static inline uint64_t get_bits_raw(Hicg64State *obj)
{
    static const uint64_t a = 1886906, b = 706715, c = 807782;
    const uint64_t x_new = a * modinv64_p2(obj->x_m1) + b * obj->x_m2 + c;
    obj->x_m2 = obj->x_m1;
    obj->x_m1 = x_new;
    return x_new;
}

static void *create(const CallerAPI *intf)
{
    Hicg64State *obj = intf->malloc(sizeof(Hicg64State));
    obj->x_m1 = intf->get_seed64() | 0x1;
    obj->x_m2 = intf->get_seed64() | 0x1;
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    Hicg64State obj = {.x_m1 = 1725239, .x_m2 = 430227};
    uint64_t u, u_ref = 0xc9337483fd17d9e7;
    for (int i = 0; i < 10000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("HICG64", run_self_test)
