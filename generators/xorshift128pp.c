/**
 * @file xorshift128pp.c
 * @brief xorshift128++ pseurorandom number generator.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna (vigna@acm.org). This PRNG is a modification of xorshift128+
 * with the output scrambler from xoroshiro128+. Such combination may
 * slightly improve a performance on AVX2.
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
 * Test values used in this code were obtained from the original xorshift128+
 * implementation. The source code for obtaining the test values can be found
 * in the `xoroshiro128pp_avx.c` PRNG.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#ifdef __AVX2__
#define XSH128PP_VEC_ENABLED
#include "smokerand/x86exts.h"
#endif

/**
 * @brief Number of xorshift128++ copies 
 */
#define NCOPIES 4


/**
 * @brief xorshift128++ PRNG state. Mustn't be initialized as (0, 0).
 */
typedef struct {
    uint64_t s[2];
} Xorshift128PPState;

/**
 * @brief xorshift128++ PRNG state: vectorized AVX2 version.
 * Its initialization must satisfy the next requirements:
 * - It mustn't be initialized with zeros!
 * - Sequences from different copies of PRNG mustn't overlap!
 */
typedef struct {
    uint64_t s0[NCOPIES];
    uint64_t s1[NCOPIES];
    uint64_t out[NCOPIES];
    size_t pos;
} Xorshift128PPVecState;



///////////////////////////////////////////
///// Scalar (cross-platform) version /////
///////////////////////////////////////////

static inline uint64_t get_bits_scalar_raw(void *state)
{
    Xorshift128PPState *obj = state;
    const uint64_t s0 = obj->s[1];
    uint64_t s1 = obj->s[0];
    const uint64_t result = rotl64(s0 + s1, 17) + s0;
    obj->s[0] = s0;
    s1 ^= s1 << 23;
    obj->s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return result;
}

MAKE_GET_BITS_WRAPPERS(scalar)

static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    Xorshift128PPState *obj = intf->malloc(sizeof(Xorshift128PPState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64() | 0x1;
    (void) gi;
    return obj;
}


static int run_self_test_scalar(const CallerAPI *intf)
{
    static const uint64_t u_ref = 0x6FE47D100616A12F;
    Xorshift128PPState gen;
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

/**
 * @brief Simultaneously processes 4 xoroshiro128++ copies by means of
 * SIMD instructions.
 */
static inline void xs128pp_block4(uint64_t *outary, uint64_t *s0ary, uint64_t *s1ary)
{
#ifdef XSH128PP_VEC_ENABLED
    __m256i s0 = _mm256_loadu_si256((__m256i *) (void *) s1ary);
    __m256i s1 = _mm256_loadu_si256((__m256i *) (void *) s0ary);
    // Apply output function to the state
    __m256i out = mm256_rotl_epi64_def(_mm256_add_epi64(s1, s0), 17);
    out = _mm256_add_epi64(out, s0);
    _mm256_storeu_si256((__m256i *) (void *) outary, out);
    // Transition to the next state
    s1 = _mm256_xor_si256(s1, _mm256_slli_epi64(s1, 23)); // s1 ^= s1 << 23
    s1 = _mm256_xor_si256(s1, _mm256_srli_epi64(s1, 18)); // s1 ^= (s1 >> 18)
    s1 = _mm256_xor_si256(s1, s0); // s1 ^= s0
    s1 = _mm256_xor_si256(s1, _mm256_srli_epi64(s0, 5)); // s1 ^= (s0 >> 5)
    // Save the new state
    _mm256_storeu_si256((__m256i *) (void *) s0ary, s0);
    _mm256_storeu_si256((__m256i *) (void *) s1ary, s1);
#else
    (void) outary; (void) s0ary; (void) s1ary;
#endif
}


void Xorshift128PPVecState_block(Xorshift128PPVecState *obj)
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

static inline uint64_t get_bits_vector_raw(void *state)
{
    Xorshift128PPVecState *obj = state;
    if (obj->pos >= NCOPIES) {
        Xorshift128PPVecState_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


MAKE_GET_BITS_WRAPPERS(vector)

void next_scalar(uint64_t *s0_out, uint64_t *s1_out, uint64_t s0in, uint64_t s1in)
{
    uint64_t s1 = s0in;
    const uint64_t s0 = s1in;
    *s0_out = s0;
    s1 ^= s1 << 23;
    *s1_out = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
}


void jump(uint64_t *s0_out, uint64_t *s1_out, uint64_t s0_in, uint64_t s1_in)
{
    static const uint64_t JUMP[] = { 0x8a5cd789635d2dff, 0x121fd2155c472f96 };
    uint64_t s0 = 0, s1 = 0;
    for (size_t i = 0; i < sizeof(JUMP) / sizeof (*JUMP); i++) {
        for (int b = 0; b < 64; b++) {
            if (JUMP[i] & UINT64_C(1) << b) {
                s0 ^= s0_in;
                s1 ^= s1_in;
            }
            next_scalar(&s0_in, &s1_in, s0_in, s1_in);
        }
    }


    *s0_out = s0;
    *s1_out = s1;
}


static void Xorshift128PPVecState_init(Xorshift128PPVecState *obj, uint64_t s0, uint64_t s1)
{
    if (s0 == 0 && s1 == 0) {
        obj->s0[0] = 0x0123456789ABCDEFULL;
        obj->s1[0] = 0xDEADBEEFDEADBEEFULL;
    }
    for (int i = 0; i < NCOPIES - 1; i++) {
        jump(obj->s0 + i + 1, obj->s1 + i + 1, obj->s0[i], obj->s1[i]);
    }
    obj->pos = 4;
}

static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef XSH128PP_VEC_ENABLED
    Xorshift128PPVecState *obj = intf->malloc(sizeof(Xorshift128PPVecState));
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    Xorshift128PPVecState_init(obj, s0, s1);
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
    static const uint64_t out_ref[] = {
        0x6FE47D100616A12F, 0x89E1B1A462268CBE,
        0xD2746B80454551B4, 0x191D440127FCE519,
        0x4C590084652BE632, 0x939AA3C35905D472,
        0x2DC42C6E48FC6621, 0x27CC4AA5942E06A0};

    int is_ok = 1;
    Xorshift128PPVecState gen;
    Xorshift128PPVecState_init(&gen, 0, 0);
    for (int i = 0; i < 100000; i++) {
        Xorshift128PPVecState_block(&gen);
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
#ifdef XSH128PP_VEC_ENABLED
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
"xorshift128++ PRNG: a LFSR with some output function. The lower bits are\n"
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
        gi->name = "xorshift128++:scalar";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "vector")) {
        gi->name = "xorshift128++:vector";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else {
        gi->name = "xorshift128++:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
