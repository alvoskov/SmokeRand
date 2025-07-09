/**
 * @file xoroshiro128pp.c
 * @brief xoroshiro128++ pseurorandom number generator: scalar and vectorized
 * (AVX2) implementations.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna (vigna@acm.org). Doesn't fail matrix rank and linear complexity
 * tests.
 *
 * Two variants are implemented:
 *
 * 1. `--param=scalar` (the default one): a cross-platform scalar version.
 * 2. `--param=vector`: a vectorized AVX2 version, may be 2-3 times faster.
 *    Its output differs from the scalar version because it uses multiple
 *    copies initialized by a jump function.
 *
 * References:
 *
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
 * @copyright The xoroshiro128++ algorithm and its public domain implementation
 * are suggested by D. Blackman and S. Vigna.
 *
 * Adaptation for SmokeRand and AVX2 based vectorization:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#ifdef  __AVX2__
#define XS128PP_VEC_ENABLED
#include "smokerand/x86exts.h"
#endif

PRNG_CMODULE_PROLOG

/**
 * @brief xoroshiro128++ PRNG state. Mustn't be initialized as (0, 0).
 */
typedef struct {
    uint64_t s[2];
} Xoroshiro128PPState;

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
} Xoroshiro128PPVecState;

///////////////////////////////////////////
///// Scalar (cross-platform) version /////
///////////////////////////////////////////

static inline uint64_t get_bits_scalar_raw(void *state)
{
    Xoroshiro128PPState *obj = state;
    const uint64_t s0 = obj->s[0];
    uint64_t s1 = obj->s[1];
    const uint64_t result = rotl64(s0 + s1, 17) + s0;
    s1 ^= s0;
    obj->s[0] = rotl64(s0, 49) ^ s1 ^ (s1 << 21); // a, b
    obj->s[1] = rotl64(s1, 28); // c
    return result;
}

MAKE_GET_BITS_WRAPPERS(scalar)

static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    Xoroshiro128PPState *obj = intf->malloc(sizeof(Xoroshiro128PPState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64() | 0x1;
    (void) gi;
    return obj;
}


static int run_self_test_scalar(const CallerAPI *intf)
{
    static const uint64_t u_ref = 0x3488CF8769131D5B;
    Xoroshiro128PPState gen;
    gen.s[0] = 0x0123456789ABCDEF;
    gen.s[1] = 0xDEADBEEFDEADBEEF;
    uint64_t u;
    for (int i = 0; i < 100000; i++) {
        u = get_bits_scalar_raw(&gen);
    }
    intf->printf("Output: 0x%16.16llX; refernce value: 0x%16.16llX\n",
        u, u_ref);
    return (u == u_ref);
}

/////////////////////////////////////
///// Vectorized (AVX2) version /////
/////////////////////////////////////

#ifdef XS128PP_VEC_ENABLED
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
#endif


void Xoroshiro128PPVecState_block(Xoroshiro128PPVecState *obj)
{
#if !defined(XS128PP_VEC_ENABLED)
    (void) obj;
#elif NCOPIES == 4
    xs128pp_block4(obj->out, obj->s0, obj->s1);
#elif NCOPIES == 8
    xs128pp_block4(obj->out, obj->s0, obj->s1);
    xs128pp_block4(obj->out + 4, obj->s0 + 4, obj->s1 + 4);
#else
#error Unknown number of copies
#endif
}

static inline uint64_t get_bits_vector_raw(void *state)
{
    Xoroshiro128PPVecState *obj = state;
    if (obj->pos >= NCOPIES) {
        Xoroshiro128PPVecState_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(vector)


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


static void Xoroshiro128PPVecState_init(Xoroshiro128PPVecState *obj, uint64_t s0, uint64_t s1)
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

static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef XS128PP_VEC_ENABLED
    Xoroshiro128PPVecState *obj = intf->malloc(sizeof(Xoroshiro128PPVecState));
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    Xoroshiro128PPVecState_init(obj, s0, s1);
    (void) gi;
    return obj;    
#else
    intf->printf("AVX2 is not supported on this platform\n");
    (void) gi;
    return NULL;
#endif
}


int run_self_test_vector(const CallerAPI *intf)
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

    Xoroshiro128PPVecState gen;
    // Part 1. Checking initialization subroutine that uses the long_jump
    // function (its algorithm is taken from the original xoroshiro128++
    // implementation)
    Xoroshiro128PPVecState_init(&gen, 0, 0);
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
        Xoroshiro128PPVecState_block(&gen);
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

//////////////////////
///// Interfaces /////
//////////////////////


static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    intf->printf("----- Scalar version internal self-test -----\n");
    is_ok = is_ok & run_self_test_scalar(intf);
    intf->printf("----- Vectorized version internal self-test -----\n");
#ifdef XS128PP_VEC_ENABLED
    is_ok = is_ok & run_self_test_vector(intf);
#else
    intf->printf("AVX2 is not supported on this platform\n");
#endif
    return is_ok;
}

static void *create(const CallerAPI *intf)
{
    intf->printf("Unknown parameter '%s'\n", intf->get_param());
    return NULL;
}

static const char description[] =
"xoroshiro128++ PRNG: a LFSR with some output function. The lower bits are\n"
"rather good and don't fail linear complexity based tests. The next param\n"
"values are supported:\n"
"  scalar - cross-platform scalar version\n"
"  vector - vectorized (AVX2) version\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->create = default_create;
    gi->free = default_free;
    gi->nbits = 64;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "scalar") || !intf->strcmp(param, "")) {
        gi->name = "xoroshiro128++:scalar";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "vector")) {
        gi->name = "xoroshiro128++:vector";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else {
        gi->name = "xoroshiro128++:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
