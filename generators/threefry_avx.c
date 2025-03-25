/**
 * @file threefry_avx.c
 * @brief A vectorized implementation of Threefry4x64x72 and Threefry4x64x20
 * PRNGs based on AVX2 x86-64 extensions. These generators are based on
 * Threefish cipher/CSPRNG, this cipher is almost identical to the
 * Threefry4x64x72 PRNG.
 *
 * @details Differences from Threefish:
 *
 * 1. Reduced number of rounds in the Threefry4x64x20.
 * 2. Tweak T is always set to {0, 0, 0}.
 * 3. Counter is used as a text.
 * 4. No XORing in output generation.
 * 5. Output is permuted due to packing of several copies of blocks into
 *    AVX2 registers. It is acceptable for PRNG but unacceptable for
 *    encryption.
 *
 * References:
 *
 * 1. J. K. Salmon, M. A. Moraes, R. O. Dror and D. E. Shaw, "Parallel random
 *    numbers: As easy as 1, 2, 3," SC '11: Proceedings of 2011 International
 *    Conference for High Performance Computing, Networking, Storage and
 *    Analysis, Seattle, WA, USA, 2011, pp. 1-12.
 *    https://doi.org/10.1145/2063384.2063405.
 * 2. Random123: a Library of Counter-Based Random Number Generators
 *    https://github.com/girving/random123/blob/main/tests/kat_vectors
 * 3. https://www.schneier.com/academic/skein/threefish/
 * 4. https://pdebuyl.be/blog/2016/threefry-rng.html
 *
 * @copyright Threefish block cipher was developed by Bruce Schneier et al.
 * Threefry algorithm was proposed by J. K. Salmon, M. A. Moraes, R. O. Dror
 * and D. E. Shaw.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

/**
 * @brief Number of words per state (DON'T CHANGE!)
 */
#define NWORDS 4

/**
 * @brief Number of generator copies (DON'T CHANGE!)
 */
#define NCOPIES 4

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

PRNG_CMODULE_PROLOG

/////////////////////////////////////////////
///// Threefry/Threefish implementation /////
/////////////////////////////////////////////

/**
 * @brief Threefry4x64xN vectorized pseudorandom number generator state.
 * Designed for x86-64 AVX2 instructions set.
 */
typedef struct Tf256VecState_ {
    uint64_t k[NWORDS + 1]; ///< Key (+ extra word)
    uint64_t ctr[NCOPIES * NWORDS]; ///< Counter ("plain text")
    uint64_t out[NCOPIES * NWORDS]; ///< Output buffer
    size_t pos; ///< Current position in the output buffer.
    void (*block_func)(struct Tf256VecState_ *); ///< Scrambling/encryption function 
} Tf256VecState;

/**
 * @brief Initializes Threefry4x64 generator state.
 * @param obj Generator to be initialized
 * @param k   Pointer to a 256-bit key (seed)
 * @relates Tf256VecState
 */
static void Tf256VecState_init(Tf256VecState *obj, const uint64_t *k)
{
    static const uint64_t C240 = 0x1BD11BDAA9FC1A22ULL;
    obj->k[NWORDS] = C240;
    for (int i = 0; i < NWORDS; i++) {
        obj->k[i] = k[i];
        obj->k[NWORDS] ^= obj->k[i];
    }
    for (int i = 0; i < NCOPIES; i++) {
        obj->ctr[i] = i;
    }
    for (int i = NCOPIES; i < NCOPIES * NWORDS; i++) {
        obj->ctr[i] = 0;
    }
    obj->pos = NWORDS * NCOPIES;
}

/**
 * @brief Vectorized "rotate left" instruction for vector of 64-bit values.
 */
static inline __m256i mm256_rotl_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi64(in, r), _mm256_srli_epi64(in, 64 - r));
}

static inline void mix4v(__m256i *xv, int d1, int d2)
{
    // Not permuted
    xv[0] = _mm256_add_epi64(xv[0], xv[1]);
    xv[2] = _mm256_add_epi64(xv[2], xv[3]);
    // Permuted
    __m256i x1 = xv[1], x3 = xv[3];
    xv[3] = _mm256_xor_si256(mm256_rotl_epi64_def(x1, d1), xv[0]);
    xv[1] = _mm256_xor_si256(mm256_rotl_epi64_def(x3, d2), xv[2]);
}

/**
 * @brief Applies a round key to the 4 copies of 256-bit state.
 * @param obj Generator state
 * @param out 1024-bit buffer with the current state.
 * @param n   Ordinal of the round key.
 * @param i0  Index for R[0] for the round key.
 * @param i1  Index for R[1] for the round key.
 * @param i2  Index for R[2] for the round key.
 * @param i3  Index for R[3] for the round key.
 * @relates Tf256VecState
 */
static inline void Tf256VecState_apply_round_key(Tf256VecState *obj, __m256i *out,
    size_t n, size_t i0, size_t i1, size_t i2, size_t i3)
{
    __m256i ks0 = _mm256_set1_epi64x(obj->k[i0]);
    __m256i ks1 = _mm256_set1_epi64x(obj->k[i1]);
    __m256i ks2 = _mm256_set1_epi64x(obj->k[i2]);
    __m256i ks3 = _mm256_set1_epi64x(obj->k[i3] + n);
    out[0] = _mm256_add_epi64(out[0], ks0);
    out[1] = _mm256_add_epi64(out[1], ks1);
    out[2] = _mm256_add_epi64(out[2], ks2);
    out[3] = _mm256_add_epi64(out[3], ks3);
}

static const int Rj0[8] = {14, 52, 23,  5,  25, 46, 58, 32};
static const int Rj1[8] = {16, 57, 40, 37,  33, 12, 22, 32};

#define ROUNDS_LOW(S, S0, S1, S2, S3) { \
    Tf256VecState_apply_round_key(obj, v, (S), (S0), (S1), (S2), (S3)); \
    mix4v(v, Rj0[0], Rj1[0]); mix4v(v, Rj0[1], Rj1[1]); \
    mix4v(v, Rj0[2], Rj1[2]); mix4v(v, Rj0[3], Rj1[3]); }

#define ROUNDS_HIGH(S, S0, S1, S2, S3) { \
    Tf256VecState_apply_round_key(obj, v, (S), (S0), (S1), (S2), (S3)); \
    mix4v(v, Rj0[4], Rj1[4]); mix4v(v, Rj0[5], Rj1[5]); \
    mix4v(v, Rj0[6], Rj1[6]); mix4v(v, Rj0[7], Rj1[7]); }

#define LOAD_COUNTER __m256i v[NWORDS]; \
    v[0] = _mm256_loadu_si256((__m256i *) obj->ctr); \
    v[1] = _mm256_loadu_si256((__m256i *) (obj->ctr + 4)); \
    v[2] = _mm256_loadu_si256((__m256i *) (obj->ctr + 8)); \
    v[3] = _mm256_loadu_si256((__m256i *) (obj->ctr + 12));

#define UNLOAD_OUTPUT \
    _mm256_storeu_si256((__m256i *) obj->out,        v[0]); \
    _mm256_storeu_si256((__m256i *) (obj->out + 4),  v[1]); \
    _mm256_storeu_si256((__m256i *) (obj->out + 8),  v[2]); \
    _mm256_storeu_si256((__m256i *) (obj->out + 12), v[3]);

/**
 * @brief ThreeFry4x64x72 (ThreeFish) block scrambling function.
 * @param obj Generator state.
 * @relates Tf256VecState
 */
EXPORT void Tf256VecState_block72(Tf256VecState *obj)
{
    static const int N_ROUNDS = 72;
    LOAD_COUNTER
    ROUNDS_LOW(0,  0, 1, 2, 3)   ROUNDS_HIGH(1,  1, 2, 3, 4)
    ROUNDS_LOW(2,  2, 3, 4, 0)   ROUNDS_HIGH(3,  3, 4, 0, 1)
    ROUNDS_LOW(4,  4, 0, 1, 2)   ROUNDS_HIGH(5,  0, 1, 2, 3)
    ROUNDS_LOW(6,  1, 2, 3, 4)   ROUNDS_HIGH(7,  2, 3, 4, 0) 
    ROUNDS_LOW(8,  3, 4, 0, 1)   ROUNDS_HIGH(9,  4, 0, 1, 2)
    ROUNDS_LOW(10, 0, 1, 2, 3)   ROUNDS_HIGH(11, 1, 2, 3, 4)
    ROUNDS_LOW(12, 2, 3, 4, 0)   ROUNDS_HIGH(13, 3, 4, 0, 1)
    ROUNDS_LOW(14, 4, 0, 1, 2)   ROUNDS_HIGH(15, 0, 1, 2, 3)
    ROUNDS_LOW(16, 1, 2, 3, 4)   ROUNDS_HIGH(17, 2, 3, 4, 0)
    // Output generation
    Tf256VecState_apply_round_key(obj, v, N_ROUNDS / 4, 3, 4, 0, 1);
    UNLOAD_OUTPUT
}

/**
 * @brief ThreeFry4x64x20 block scrambling function.
 * @param obj Generator state.
 * @relates Tf256VecState
 */
EXPORT void Tf256VecState_block20(Tf256VecState *obj)
{
    static const int N_ROUNDS = 20;
    LOAD_COUNTER
    ROUNDS_LOW(0,  0, 1, 2, 3)   ROUNDS_HIGH(1,  1, 2, 3, 4)
    ROUNDS_LOW(2,  2, 3, 4, 0)   ROUNDS_HIGH(3,  3, 4, 0, 1)
    ROUNDS_LOW(4,  4, 0, 1, 2)
    // Output generation
    Tf256VecState_apply_round_key(obj, v, N_ROUNDS / 4, 0, 1, 2, 3);
    UNLOAD_OUTPUT
}

/**
 * @brief Increment PRNG counters.
 * @param obj Generator state.
 * @relates Tf256VecState
 */
static inline void Tf256VecState_inc_counter(Tf256VecState *obj)
{
    for (int i = 0; i < NCOPIES; i++) {
        obj->ctr[i] += NCOPIES;
    }
}

///////////////////////////////
///// Internal self-tests /////
///////////////////////////////


/**
 * @brief Comparison of vectors for internal self-tests.
 */
static int self_test_compare(const CallerAPI *intf,
    const uint64_t *out, const uint64_t *ref)
{
    intf->printf("OUT: ");
    int is_ok = 1;
    for (size_t i = 0; i < NCOPIES * NWORDS; i++) {
        intf->printf("%llX ", out[i]);
        if (out[i] != ref[i / NCOPIES])
            is_ok = 0;
    }
    intf->printf("\n");
    intf->printf("REF: ");
    for (size_t i = 0; i < NCOPIES * NWORDS; i++) {
        intf->printf("%llX ", ref[i / NCOPIES]);
    }
    intf->printf("\n");
    return is_ok;
}

/**
 * @brief An internal self-test. Test vectors are taken
 * from Random123 library.
 */
static int run_self_test(const CallerAPI *intf)
{
    Tf256VecState obj;
    static const uint64_t k0_m1[4] = {-1, -1, -1, -1};
    static const uint64_t ref72_m1[4] = {0x11518c034bc1ff4cull,
        0x193f10b8bcdcc9f7ull, 0xd024229cb58f20d8ull, 0x563ed6e48e05183full};
    static const uint64_t ref20_m1[4] = {0x29c24097942bba1bull,
        0x0371bbfb0f6f4e11ull, 0x3c231ffa33f83a1cull, 0xcd29113fde32d168ull};

    static const uint64_t k0_pi[4] = {0x452821e638d01377ull,
        0xbe5466cf34e90c6cull, 0xbe5466cf34e90c6c, 0xc0ac29b7c97c50ddull};
    static const uint64_t ref72_pi[4] = {0xacf412ccaa3b2270ull,
        0xc9e99bd53f2e9173ull, 0x43dad469dc825948ull, 0xfbb19d06c8a2b4dcull};
    static const uint64_t ref20_pi[4] = {0xa7e8fde591651bd9ull,
        0xbaafd0c30138319bull, 0x84a5c1a729e685b9ull, 0x901d406ccebc1ba4ull};

    Tf256VecState_init(&obj, k0_m1);
    for (int i = 0; i < NWORDS * NCOPIES; i++) {
        obj.ctr[i] = -1;
    }
    intf->printf("Threefry4x64x72 ('-1' example)\n");
    Tf256VecState_block72(&obj);
    if (!self_test_compare(intf, obj.out, ref72_m1)) {
        return 0;
    }
    intf->printf("Threefry4x64x20 ('-1' example)\n");
    Tf256VecState_block20(&obj);
    if (!self_test_compare(intf, obj.out, ref20_m1)) {
        return 0;
    }
    Tf256VecState_init(&obj, k0_pi);
    for (int i = 0; i < NCOPIES; i++) {
        obj.ctr[i]            = 0x243f6a8885a308d3;
        obj.ctr[i +   NWORDS] = 0x13198a2e03707344;
        obj.ctr[i + 2*NWORDS] = 0xa4093822299f31d0;
        obj.ctr[i + 3*NWORDS] = 0x082efa98ec4e6c89;
    }

    intf->printf("Threefry4x64x72 ('pi' example)\n");
    Tf256VecState_block72(&obj);
    if (!self_test_compare(intf, obj.out, ref72_pi)) {
        return 0;
    }
    intf->printf("Threefry4x64x20 ('pi' example)\n");
    Tf256VecState_block20(&obj);
    if (!self_test_compare(intf, obj.out, ref20_pi)) {
        return 0;
    }
    return 1;
}


/////////////////////////////////////
///// Module external interface /////
/////////////////////////////////////

static inline uint64_t get_bits_raw(void *state)
{
    Tf256VecState *obj = state;
    if (obj->pos >= NWORDS * NCOPIES) {
        Tf256VecState_inc_counter(obj);
        obj->block_func(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


static void *create(const CallerAPI *intf)
{
    uint64_t k[NWORDS];
    Tf256VecState *obj = intf->malloc(sizeof(Tf256VecState));
    for (size_t i = 0; i < NWORDS; i++) {
        k[i] = intf->get_seed64();
    }
    Tf256VecState_init(obj, k);
    const char *param = intf->get_param();
    if (!intf->strcmp(param, "ThreeFry") || !intf->strcmp(param, "")) {
        intf->printf("ThreeFry4x64x20\n", param);
        obj->block_func = Tf256VecState_block20;
    } else if (!intf->strcmp(param, "ThreeFish")) {
        intf->printf("ThreeFry4x64x72 (ThreeFish)\n", param);
        obj->block_func = Tf256VecState_block72;
    } else {
        intf->printf("Unknown parameter '%s'\n", param);
        intf->free(obj);
        return NULL;
    }
    return (void *) obj;
}


MAKE_UINT64_PRNG("Threefry4x64_AVX", run_self_test)
