/**
 * @file pcg128
 * @brief pcg_oneseq_128_xsh_rs_64 (permuted conguential generator)
 * by Melissa O'Neill.
 * @details Spectral test results for the multiplier:
 *
 *    t = 2; mu = 2.4863; fd = .8278; log2(v) = 63;
 *    t = 3; mu = 1.5771; fd = .6433; log2(v) = 42;
 *    t = 4; mu = 2.2601; fd = .6917; log2(v) = 31;
 *    t = 5; mu = 1.8806; fd = .6611; log2(v) = 25;
 *    t = 6; mu = 3.4923; fd = .7259; log2(v) = 21;
 *    t = 7; mu = 1.8534; fd = .6500; log2(v) = 18;
 *    t = 8; mu = 2.0975; fd = .6511; log2(v) = 15;
 *    Merit = .6433; Merit_H = .7295
 *
 * Python 3.x script for generation of test vectors for the internal
 * self-test.
 *
 *    a = 2549297995355413924*2**64 + 4865540595714422341
 *    c = 6364136223846793005*2**64 + 1442695040888963407
 *    x = 12345 * 2**64 + 54321
 *    for i in range(1_000_000):
 *        x = (a*x + c) % 2**128
 *        # Good scheme
 *        out = ( ((x >> 43) ^ x) >> ((x >> 124) + 45) ) % 2**64
 *        # C-style
 *        hi, lo = x >> 64, x % 2**64
 *        sh = (hi >> 60) + 45
 *        xs_hi, xs_lo = hi ^ (hi >> 43), (lo ^ (hi << 21)) % 2**64 # (x>>43)^x
 *        out_c99 = ( (xs_hi << (64 - sh)) | (xs_lo >> sh) ) % 2**64
 *        if i < 16:
 *            print(i, "out int128 style: ", hex(out))
 *            print(i, "out c99 style:    ", hex(out_c99))
 * 
 *    print(hex(out), hex(out_c99), a % 8)
 *
 * References:
 *
 * 1. https://github.com/imneme/pcg-c/blob/master/include/pcg_variants.h
 * 2. https://www.pcg-random.org/
 * 3. https://pcg.di.unimi.it/pcg128-speed.c
 *
 * @copyright PCG (permuted congruential generators) PRNG family was
 * developed by Melissa O'Neill
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg128State *obj)
{
    static const uint64_t a_hi = 2549297995355413924ULL;
    static const uint64_t a_lo = 4865540595714422341ULL;
    static const uint64_t c_hi = 6364136223846793005ULL;
    static const uint64_t c_lo = 1442695040888963407ULL;
    // LCG state increment
    Lcg128State_a128_iter(obj, a_hi, a_lo, c_lo);
    obj->x_high += c_hi;
    // Output function
    // xs = (x >> 43) ^ x
    const uint64_t xs_hi = obj->x_high ^ (obj->x_high >> 43);
    const uint64_t xs_lo = obj->x_low  ^ (obj->x_high << 21);
    // xs >> (x >> 124) + 45)
    const unsigned int sh = (unsigned int) ((obj->x_high >> 60U) + 45U);
    return (xs_hi << (64U - sh)) | (xs_lo >> sh);
}


static void *create(const CallerAPI *intf)
{
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));
    Lcg128State_seed(obj, intf);
    return obj;
}

/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
    Lcg128State obj = {.x_low = 54321, .x_high = 12345};
    uint64_t u, u_ref = 0x26159d47a8550c35;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("PCG128-XSH-RS-64", run_self_test)
