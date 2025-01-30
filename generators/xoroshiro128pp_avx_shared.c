/**
 * @file xoroshiro128pp_avx_shared.c
 * @brief xoroshiro128++ pseurorandom number generator.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna (vigna@acm.org). This PRNG fails matrixrank and linearcomp tests.
 *
 * References:
 * 1. D. Blackman, S. Vigna. Scrambled Linear Pseudorandom Number Generators
 *    // ACM Transactions on Mathematical Software (TOMS). 2021. V. 47. N 4.
 *    Article No.: 36, P.1-32. https://doi.org/10.1145/3460772
 * 2. D. Lemire, M. E. O'Neill. Xorshift1024*, xorshift1024+, xorshift128+
 *    and xoroshiro128+ fail statistical tests for linearity // Journal of
 *    Computational and Applied Mathematics. 2019. V.350. P.139-142.
 *    https://doi.org/10.1016/j.cam.2018.10.019.
 * 3. xoshiro / xoroshiro generators and the PRNG shootout
 *    https://prng.di.unimi.it/
 *
 * Test values used in this code were obtained from the original xoroshiro128++
 * implementation by means of the next source code:
 *
 *     void next_value()
 *     {
 *         uint64_t scopy[2] = {s[0], s[1]}, u;
 *         for (int i = 0; i < 100000; i++) {
 *             u = next();
 *         }
 *         printf("  OUT: %llX\n", u);
 *         s[0] = scopy[0]; s[1] = scopy[1];
 *     }
 *
 *     int main()
 *     {
 *         s[0] = 0x0123456789ABCDEF; s[1] = 0xDEADBEEFDEADBEEF;
 *         next_value();
 *         for (int i = 0; i < 7; i++) {
 *             long_jump();
 *             printf("%llX %llX\n", s[0], s[1]);
 *             next_value();
 *         }
 *         return 0;
 *     }
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

/**
 * @brief Number of xoroshiro128++ copies 
 */
#define NCOPIES 4

/**
 * @brief xoroshiro128++ PRNG state: vectorized AVX2 version.
 * Its initialization must satisfy the next requirements:
 * - It mustn't be initialized with zeros!
 * - Sequences from different copies of PRNG mustn't overlap!
 */
typedef struct {
    uint64_t s0[NCOPIES];
    uint64_t s1[NCOPIES];
    uint64_t out[NCOPIES];
    size_t pos;
} Xoroshiro128PPAVXState;


/**
 * @brief Vectorized "rotate left" instruction for vector of 64-bit values.
 */
static inline __m256i mm256_rotl_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi64(in, r), _mm256_srli_epi64(in, 64 - r));
}


/**
 * @brief Simultaneously processes 4 xoroshiro128++ copies by means of
 * SIMD instructions.
 */
static inline void xs128pp_block4(uint64_t *outary, uint64_t *s0ary, uint64_t *s1ary)
{
    __m256i s0 = _mm256_loadu_si256((__m256i *) s0ary);
    __m256i s1 = _mm256_loadu_si256((__m256i *) s1ary);
    // Apply output function to the state
    // out = rotl(s0, s1, 17) + s0
    __m256i out = _mm256_add_epi64(s1, s0);
    out = mm256_rotl_epi64_def(out, 17);
    out = _mm256_add_epi64(out, s0);
    _mm256_storeu_si256((__m256i *) outary, out);
    // Transition to the next state
    s1 = _mm256_xor_si256(s1, s0); // s1 ^= s0
    s0 = mm256_rotl_epi64_def(s0, 49); // s0 = rotl(s0, 49)
    s0 = _mm256_xor_si256(s0, s1); // s0 ^= s1
    s0 = _mm256_xor_si256(s0, _mm256_slli_epi64(s1, 21)); // s0 ^= (s1 << 21)
    s1 = mm256_rotl_epi64_def(s1, 28);
    // Save the new state
    _mm256_storeu_si256((__m256i *) s0ary, s0);
    _mm256_storeu_si256((__m256i *) s1ary, s1);
}


void Xoroshiro128PPAVXState_block(Xoroshiro128PPAVXState *obj)
{
#if NCOPIES == 4
    xs128pp_block4(obj->out, obj->s0, obj->s1);
#elif NCOPIES == 8
    xs128pp_block4(obj->out, obj->s0, obj->s1);
    xs128pp_block4(obj->out + 4, obj->s0 + 4, obj->s1 + 4);
#else
#error Unknown number of copies
#endif
}

static inline uint64_t get_bits_raw(void *state)
{
    Xoroshiro128PPAVXState *obj = state;
    if (obj->pos >= NCOPIES) {
        Xoroshiro128PPAVXState_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
    (void) state;
    return 0;
}


void next_scalar(uint64_t *s0_out, uint64_t *s1_out, uint64_t s0, uint64_t s1)
{
    s1 ^= s0;
    *s0_out = rotl64(s0, 49) ^ s1 ^ (s1 << 21); // a, b
    *s1_out = rotl64(s1, 28); // c
}


void long_jump(uint64_t *s0_out, uint64_t *s1_out, uint64_t s0_in, uint64_t s1_in)
{
    static const uint64_t LONG_JUMP[] = { 0x360fd5f2cf8d5d99, 0x9c6e6877736c46e3 };
    uint64_t s0 = 0, s1 = 0;
    for (size_t i = 0; i < sizeof(LONG_JUMP) / sizeof (*LONG_JUMP); i++) {
        for (int b = 0; b < 64; b++) {
            if (LONG_JUMP[i] & UINT64_C(1) << b) {
                s0 ^= s0_in;
                s1 ^= s1_in;
            }
            next_scalar(&s0_in, &s1_in, s0_in, s1_in);
        }
    }


    *s0_out = s0;
    *s1_out = s1;
}


static void Xoroshiro128PPAVXState_init(Xoroshiro128PPAVXState *obj, uint64_t s0, uint64_t s1)
{
    if (s0 == 0 && s1 == 0) {
        obj->s0[0] = 0x0123456789ABCDEFULL;
        obj->s1[0] = 0xDEADBEEFDEADBEEFULL;
    }
    for (int i = 0; i < NCOPIES - 1; i++) {
        long_jump(obj->s0 + i + 1, obj->s1 + i + 1, obj->s0[i], obj->s1[i]);
    }
    obj->pos = 4;
}

static void *create(const CallerAPI *intf)
{
    Xoroshiro128PPAVXState *obj = intf->malloc(sizeof(Xoroshiro128PPAVXState));
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    Xoroshiro128PPAVXState_init(obj, s0, s1);
    return (void *) obj;    
}


static int run_self_test(const CallerAPI *intf)
{
    // Reference values obtained from the original implementation of
    // the xorshift128++ PRNG
    static const uint64_t s0_ref[] = {
        0x0123456789ABCDEF, 0xE335DFC015BF19A9,
        0xAE1A992F86850AA0, 0x7C4F5A166D70AB56,
        0xD4914F740DB43EB2, 0x5B8260C60E0D66D3, 
        0x412EF3C4ACFB1B2F, 0xF3118290D8C91092};

    static const uint64_t s1_ref[] = {
        0xDEADBEEFDEADBEEF, 0xAFED47A081CAAC85,
        0x0AF215101313B19C, 0x0BF13C30B39A0333,
        0x1D7353D6B628A7FE, 0xE0BB7B53B17F3989,
        0xA4D671F6D2E828EB, 0x2A25045F664D626C};

    static const uint64_t out_ref[] = {
        0x3488CF8769131D5B, 0x5FB0EC86B1916AEA,
        0xD29D03760626428F, 0x299591D612922150,
        0x43371470CAA42BFC, 0xCC178783DD4ABF9D,
        0x49F7CAA1C393FB39, 0xDCB5FA141B63D33C};

    Xoroshiro128PPAVXState gen;
    // Part 1. Checking initialization subroutine that uses the long_jump
    // function (its algorithm is taken from the original xoroshiro128++
    // implementation)
    Xoroshiro128PPAVXState_init(&gen, 0, 0);
    intf->printf("%16s %16s | %16s %16s\n",
        "s0out", "s1out", "s0ref", "s1ref");
    int is_ok = 1;
    for (int i = 0; i < NCOPIES; i++) {
        intf->printf("%16.16llX %16.16llX | %16.16llX %16.16llX\n",
            (unsigned long long) gen.s0[i],
            (unsigned long long) gen.s1[i],
            (unsigned long long) s0_ref[i],
            (unsigned long long) s1_ref[i]);
        if (gen.s0[i] != s0_ref[i] || gen.s1[i] != s1_ref[i]) {
            is_ok = 0;
        }
    }
    // Part 2. Checking the generator output
    for (int i = 0; i < NCOPIES; i++) {
        gen.s0[i] = s0_ref[i];
        gen.s1[i] = s1_ref[i];
    }
    for (int i = 0; i < 100000; i++) {
        Xoroshiro128PPAVXState_block(&gen);
    }
    intf->printf("%16s %16s\n", "out", "out(ref)");
    for (int i = 0; i < NCOPIES; i++) {
        intf->printf("%16.16llX | %16.16llX\n",
            (unsigned long long) gen.out[i],
            (unsigned long long) out_ref[i]);
        if (gen.out[i] != out_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("xoroshiro128++AVX", run_self_test)
