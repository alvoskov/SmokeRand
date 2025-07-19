/**
 * @file threefry.c
 * @brief An implementation of Threefry4x64x72 and Threefry4x64x20 PRNGs.
 * They are based on Threefish cipher/CSPRNG, this cipher is almost identical
 * to the Threefry4x64x72 PRNG.
 *
 * @details This module contains both scalar and vectorized implementations
 * of Threefry4x64x72 and Threefry4x64x20 (`threefry`, `threefry-avx2`,
 * `threefish`, `threefish-avx2` parameter values). The vectorized ones are 
 * PRNGs based on AVX2 x86-64 extensions.
 *
 * Differences from Threefish:
 *
 * 1. Reduced number of rounds in the Threefry4x64x20.
 * 2. Tweak T is always set to {0, 0, 0}.
 * 3. Counter is used as a text.
 * 4. No XORing in output generation.
 * 5. Output of vectorized (AVX2) is permuted due to packing of several copies
 *    of blocks into AVX2 registers. It is acceptable for PRNG but unacceptable
 *    for encryption.
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

#ifdef  __AVX2__
#define THREEFRY_VEC_ENABLED
#include "smokerand/x86exts.h"
#endif

enum {
    NWORDS = 4, ///< Number of words per state (DON'T CHANGE!)
    NCOPIES = 4, ///< Number of generator copies (DON'T CHANGE!)
    C240 = 0x1BD11BDAA9FC1A22ULL
};


typedef enum {
    TF256_UNKNOWN,
    TF256_THREEFRY_SCALAR,
    TF256_THREEFRY_AVX2,
    TF256_THREEFISH_SCALAR,
    TF256_THREEFISH_AVX2
} Tf256Type;

typedef struct {
    Tf256Type type;
    const char *name;
} Tf256Info;

static Tf256Info Tf256Info_get(const CallerAPI *intf)
{
    const char *param = intf->get_param();
    if (!intf->strcmp(param, "threefry") || !intf->strcmp(param, "")) {
        Tf256Info ti = {TF256_THREEFRY_SCALAR, "Threefry4x64x20"};
        return ti;
    } else if (!intf->strcmp(param, "threefish")) {
        Tf256Info ti = {TF256_THREEFISH_SCALAR, "Threefry4x64x72 (Threefish)"};
        return ti;
    } else if (!intf->strcmp(param, "threefry-avx2")) {
        Tf256Info ti = {TF256_THREEFRY_AVX2, "Threefry4x64x20:AVX2"};
        return ti;
    } else if (!intf->strcmp(param, "threefish-avx2")) {
        Tf256Info ti = {TF256_THREEFISH_AVX2, "Threefry4x64x72:AVX2 (Threefish)"};
        return ti;
    } else {
        Tf256Info ti = {TF256_UNKNOWN, "Threefry4x64:unknown"};
        return ti;
    }
}

static const int Rj0[8] = {14, 52, 23,  5,  25, 46, 58, 32};
static const int Rj1[8] = {16, 57, 40, 37,  33, 12, 22, 32};

PRNG_CMODULE_PROLOG

////////////////////////////////////////////////////
///// Threefry/Threefish scalar implementation /////
////////////////////////////////////////////////////

/**
 * @brief Threefry4x64xN scalar pseudorandom number generator state, it doesn't
 * rely on SIMD and is cross-platform.
 */
typedef struct Tf256State_ {
    BufGen64Interface intf;
    uint64_t k[NWORDS + 1]; /// Key (+ extra word)
    uint64_t p[NWORDS]; /// Counter ("plain text")
    uint64_t v[NWORDS]; /// Output buffer
} Tf256State;

/**
 * @brief Threefish/Threefry mixing function.
 */
static inline void mix4(uint64_t *x, int d1, int d2)
{
    // Not permuted
    x[0] = x[0] + x[1];
    x[2] = x[2] + x[3];
    // Permuted
    uint64_t x1 = x[1], x3 = x[3];
    x[3] = ( (x1 << d1) | (x1 >> (64 - d1)) ) ^ x[0];
    x[1] = ( (x3 << d2) | (x3 >> (64 - d2)) ) ^ x[2];
}

/**
 * @brief Generates round keys for Threefry and Threefish.
 * @memberof Tf256State_
 */
static inline void Tf256State_key_schedule(Tf256State *obj, uint64_t *out,
    size_t s, size_t s0, size_t s1, size_t s2, size_t s3)
{    
    out[0] += obj->k[s0];
    out[1] += obj->k[s1];
    out[2] += obj->k[s2];
    out[3] += obj->k[s3] + s;
}

#define ROUNDS_LOW(S, S0, S1, S2, S3) { \
    Tf256State_key_schedule(obj, v, (S), (S0), (S1), (S2), (S3)); \
    mix4(v, Rj0[0], Rj1[0]); mix4(v, Rj0[1], Rj1[1]); \
    mix4(v, Rj0[2], Rj1[2]); mix4(v, Rj0[3], Rj1[3]); }

#define ROUNDS_HIGH(S, S0, S1, S2, S3) { \
    Tf256State_key_schedule(obj, v, (S), (S0), (S1), (S2), (S3)); \
    mix4(v, Rj0[4], Rj1[4]); mix4(v, Rj0[5], Rj1[5]); \
    mix4(v, Rj0[6], Rj1[6]); mix4(v, Rj0[7], Rj1[7]); }


/**
 * @brief Generate the 256-bit block using a simplified 72-round version
 * of ThreeFish.
 * @memberof Tf256State_
 */
EXPORT void Tf256State_block72(Tf256State *obj)
{
    static const int N_ROUNDS = 72;
    uint64_t v[NWORDS];
    for (size_t i = 0; i < NWORDS; i++) {
        v[i] = obj->p[i];
    }

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
    Tf256State_key_schedule(obj, v, N_ROUNDS / 4, 3, 4, 0, 1);
    for (size_t i = 0; i < NWORDS; i++) {
        obj->v[i] = v[i];
    }
}

/**
 * @brief Generate the 256-bit block using Threefry (20-round version
 * of ThreeFish)
 * @memberof Tf256State_
 */
EXPORT void Tf256State_block20(Tf256State *obj)
{
    static const int N_ROUNDS = 20;
    uint64_t v[NWORDS];
    for (size_t i = 0; i < NWORDS; i++) {
        v[i] = obj->p[i];
    }

    ROUNDS_LOW(0,  0, 1, 2, 3)   ROUNDS_HIGH(1,  1, 2, 3, 4)
    ROUNDS_LOW(2,  2, 3, 4, 0)   ROUNDS_HIGH(3,  3, 4, 0, 1)
    ROUNDS_LOW(4,  4, 0, 1, 2)

    // Output generation
    Tf256State_key_schedule(obj, v, N_ROUNDS / 4, 0, 1, 2, 3);
    for (size_t i = 0; i < NWORDS; i++) {
        obj->v[i] = v[i];
    }
}


/**
 * @brief Increases an internal counter, required for generation
 * of the next block.
 * @memberof Tf256State_
 */
static inline void Tf256State_inc_counter(Tf256State *obj)
{
    if (++obj->p[0] == 0) obj->p[1]++;
}

/**
 * @brief Generates a new block of pseudorandom numbers using the 72-round
 * simplified version of ThreeFish and updates internal counters.
 * @memberof Tf256State_
 */
void Tf256State_iter_func_72(void *data)
{
    Tf256State *obj = data;
    Tf256State_block72(obj);
    Tf256State_inc_counter(obj);
    obj->intf.pos = 0;
}

/**
 * @brief Generates a new block of pseudorandom numbers using the 20-round
 * version of ThreeFish (ThreeFry) and updates internal counters.
 * @memberof Tf256State_
 */
void Tf256State_iter_func_20(void *data)
{
    Tf256State *obj = data;
    Tf256State_block20(obj);
    Tf256State_inc_counter(obj);
    obj->intf.pos = 0;
}

/**
 * @brief Initialized the Threefry/Threefish pseudorandom number generator
 * internal state.
 * @param obj      Generator state to be initialized.
 * @param k        Pointer to the 256-bit key (seed).
 * @param is_full  0 - initialize as Threefry, 1 - initialize as Threefish. 
 * @memberof Tf256State_
 */
static void Tf256State_init(Tf256State *obj, const uint64_t *k, int is_full)
{
    obj->k[NWORDS] = C240;
    for (size_t i = 0; i < NWORDS; i++) {
        obj->k[i] = k[i];
        obj->p[i] = 0;
        obj->k[NWORDS] ^= obj->k[i];
    }
    if (is_full) {
        obj->intf.iter_func = Tf256State_iter_func_72;
    } else {
        obj->intf.iter_func = Tf256State_iter_func_20;
    }
    obj->intf.pos = NWORDS;
    obj->intf.bufsize = NWORDS;
    obj->intf.out = obj->v;
}

///////////////////////////////////////////////////////////////
///// Threefry/Threefish vectorized (AVX2) implementation /////
///////////////////////////////////////////////////////////////

/**
 * @brief Threefry4x64xN vectorized pseudorandom number generator state.
 * Designed for x86-64 AVX2 instructions set.
 */
typedef struct Tf256VecState_ {
    BufGen64Interface intf;
    uint64_t k[NWORDS + 1]; ///< Key (+ extra word)
    uint64_t ctr[NCOPIES * NWORDS]; ///< Counter ("plain text")
    uint64_t out[NCOPIES * NWORDS]; ///< Output buffer
} Tf256VecState;


#ifdef THREEFRY_VEC_ENABLED
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

#define ROUNDS_LOW_VEC(S, S0, S1, S2, S3) { \
    Tf256VecState_apply_round_key(obj, v, (S), (S0), (S1), (S2), (S3)); \
    mix4v(v, Rj0[0], Rj1[0]); mix4v(v, Rj0[1], Rj1[1]); \
    mix4v(v, Rj0[2], Rj1[2]); mix4v(v, Rj0[3], Rj1[3]); }

#define ROUNDS_HIGH_VEC(S, S0, S1, S2, S3) { \
    Tf256VecState_apply_round_key(obj, v, (S), (S0), (S1), (S2), (S3)); \
    mix4v(v, Rj0[4], Rj1[4]); mix4v(v, Rj0[5], Rj1[5]); \
    mix4v(v, Rj0[6], Rj1[6]); mix4v(v, Rj0[7], Rj1[7]); }

#define LOAD_COUNTER_VEC __m256i v[NWORDS]; \
    v[0] = _mm256_loadu_si256((__m256i *) obj->ctr); \
    v[1] = _mm256_loadu_si256((__m256i *) (obj->ctr + 4)); \
    v[2] = _mm256_loadu_si256((__m256i *) (obj->ctr + 8)); \
    v[3] = _mm256_loadu_si256((__m256i *) (obj->ctr + 12));

#define UNLOAD_OUTPUT_VEC \
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
    LOAD_COUNTER_VEC
    ROUNDS_LOW_VEC(0,  0, 1, 2, 3)   ROUNDS_HIGH_VEC(1,  1, 2, 3, 4)
    ROUNDS_LOW_VEC(2,  2, 3, 4, 0)   ROUNDS_HIGH_VEC(3,  3, 4, 0, 1)
    ROUNDS_LOW_VEC(4,  4, 0, 1, 2)   ROUNDS_HIGH_VEC(5,  0, 1, 2, 3)
    ROUNDS_LOW_VEC(6,  1, 2, 3, 4)   ROUNDS_HIGH_VEC(7,  2, 3, 4, 0) 
    ROUNDS_LOW_VEC(8,  3, 4, 0, 1)   ROUNDS_HIGH_VEC(9,  4, 0, 1, 2)
    ROUNDS_LOW_VEC(10, 0, 1, 2, 3)   ROUNDS_HIGH_VEC(11, 1, 2, 3, 4)
    ROUNDS_LOW_VEC(12, 2, 3, 4, 0)   ROUNDS_HIGH_VEC(13, 3, 4, 0, 1)
    ROUNDS_LOW_VEC(14, 4, 0, 1, 2)   ROUNDS_HIGH_VEC(15, 0, 1, 2, 3)
    ROUNDS_LOW_VEC(16, 1, 2, 3, 4)   ROUNDS_HIGH_VEC(17, 2, 3, 4, 0)
    // Output generation
    Tf256VecState_apply_round_key(obj, v, N_ROUNDS / 4, 3, 4, 0, 1);
    UNLOAD_OUTPUT_VEC
}

/**
 * @brief ThreeFry4x64x20 block scrambling function.
 * @param obj Generator state.
 * @relates Tf256VecState
 */
EXPORT void Tf256VecState_block20(Tf256VecState *obj)
{
    static const int N_ROUNDS = 20;
    LOAD_COUNTER_VEC
    ROUNDS_LOW_VEC(0,  0, 1, 2, 3)   ROUNDS_HIGH_VEC(1,  1, 2, 3, 4)
    ROUNDS_LOW_VEC(2,  2, 3, 4, 0)   ROUNDS_HIGH_VEC(3,  3, 4, 0, 1)
    ROUNDS_LOW_VEC(4,  4, 0, 1, 2)
    // Output generation
    Tf256VecState_apply_round_key(obj, v, N_ROUNDS / 4, 0, 1, 2, 3);
    UNLOAD_OUTPUT_VEC
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


/**
 * @brief Generates a new block of pseudorandom numbers and updates
 * internal counters.
 * @relates Tf256VecState
 */
void Tf256VecState_iter_func_72(void *data)
{
    Tf256VecState *obj = data;
    Tf256VecState_block72(obj);
    Tf256VecState_inc_counter(obj);
    obj->intf.pos = 0;
}

/**
 * @brief Generates a new block of pseudorandom numbers and updates
 * internal counters.
 * @relates Tf256VecState
 */
void Tf256VecState_iter_func_20(void *data)
{
    Tf256VecState *obj = data;
    Tf256VecState_block20(obj);
    Tf256VecState_inc_counter(obj);
    obj->intf.pos = 0;
}


/**
 * @brief Initializes Threefry4x64 generator state.
 * @param obj Generator to be initialized
 * @param k   Pointer to a 256-bit key (seed)
 * @relates Tf256VecState
 */
static void Tf256VecState_init(Tf256VecState *obj, const uint64_t *k, int is_full)
{
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
    if (is_full) {
        obj->intf.iter_func = Tf256VecState_iter_func_72;
    } else {
        obj->intf.iter_func = Tf256VecState_iter_func_20;
    }
    obj->intf.pos = NWORDS * NCOPIES;
    obj->intf.bufsize = NWORDS * NCOPIES;
    obj->intf.out = obj->out;
}             

#endif



////////////////////////////////////////////////////////////////////////
///// Internal self-tests for scalar version of Threefry/Threefish /////
////////////////////////////////////////////////////////////////////////


/**
 * @brief Comparison of vectors for internal self-tests.
 */
static int self_test_compare(const CallerAPI *intf,
    const uint64_t *out, const uint64_t *ref)
{
    intf->printf("OUT: ");
    int is_ok = 1;
    for (size_t i = 0; i < NWORDS; i++) {
        intf->printf("%llX ", out[i]);
        if (out[i] != ref[i])
            is_ok = 0;
    }
    intf->printf("\n");
    intf->printf("REF: ");
    for (size_t i = 0; i < NWORDS; i++) {
        intf->printf("%llX ", ref[i]);
    }
    intf->printf("\n");
    return is_ok;
}

/**
 * @brief An internal self-test. Test vectors are taken
 * from Random123 library.
 */
static int run_self_test_scalar(const CallerAPI *intf)
{
    Tf256State obj;
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

    Tf256State_init(&obj, k0_m1, 1);
    obj.p[0] = -1; obj.p[1] = -1;
    obj.p[2] = -1; obj.p[3] = -1;

    intf->printf("Threefry4x64x72 ('-1' example)\n");
    Tf256State_block72(&obj);
    if (!self_test_compare(intf, obj.v, ref72_m1)) {
        return 0;
    }
    intf->printf("Threefry4x64x20 ('-1' example)\n");
    Tf256State_block20(&obj);
    if (!self_test_compare(intf, obj.v, ref20_m1)) {
        return 0;
    }

    Tf256State_init(&obj, k0_pi, 1);
    obj.p[0] = 0x243f6a8885a308d3; obj.p[1] = 0x13198a2e03707344;
    obj.p[2] = 0xa4093822299f31d0; obj.p[3] = 0x082efa98ec4e6c89;

    intf->printf("Threefry4x64x72 ('pi' example)\n");
    Tf256State_block72(&obj);
    if (!self_test_compare(intf, obj.v, ref72_pi)) {
        return 0;
    }
    intf->printf("Threefry4x64x20 ('pi' example)\n");
    Tf256State_block20(&obj);
    if (!self_test_compare(intf, obj.v, ref20_pi)) {
        return 0;
    }
    return 1;
}

/**
 * @brief Comparison of vectors for internal self-tests.
 */
int self_test_compare_vec(const CallerAPI *intf,
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
static int run_self_test_vector(const CallerAPI *intf)
{
#ifdef THREEFRY_VEC_ENABLED
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

    Tf256VecState_init(&obj, k0_m1, 1);
    for (int i = 0; i < NWORDS * NCOPIES; i++) {
        obj.ctr[i] = -1;
    }
    intf->printf("Threefry4x64x72 ('-1' example)\n");
    Tf256VecState_block72(&obj);
    if (!self_test_compare_vec(intf, obj.out, ref72_m1)) {
        return 0;
    }
    intf->printf("Threefry4x64x20 ('-1' example)\n");
    Tf256VecState_block20(&obj);
    if (!self_test_compare_vec(intf, obj.out, ref20_m1)) {
        return 0;
    }
    Tf256VecState_init(&obj, k0_pi, 1);
    for (int i = 0; i < NCOPIES; i++) {
        obj.ctr[i]            = 0x243f6a8885a308d3;
        obj.ctr[i +   NWORDS] = 0x13198a2e03707344;
        obj.ctr[i + 2*NWORDS] = 0xa4093822299f31d0;
        obj.ctr[i + 3*NWORDS] = 0x082efa98ec4e6c89;
    }

    intf->printf("Threefry4x64x72 ('pi' example)\n");
    Tf256VecState_block72(&obj);
    if (!self_test_compare_vec(intf, obj.out, ref72_pi)) {
        return 0;
    }
    intf->printf("Threefry4x64x20 ('pi' example)\n");
    Tf256VecState_block20(&obj);
    if (!self_test_compare_vec(intf, obj.out, ref20_pi)) {
        return 0;
    }
    return 1;
#else
    intf->printf("AVX2 vectorized version is not implemented on this platform\n");
    return 1;
#endif
}



static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    intf->printf("----- Scalar version -----\n");
    is_ok = is_ok & run_self_test_scalar(intf);
    intf->printf("----- Vectorized (AVX2) version -----\n");
    is_ok = is_ok & run_self_test_vector(intf);
    return is_ok;
}

/////////////////////////////////////
///// Module external interface /////
/////////////////////////////////////


/*
// This variant is about 20% faster for Threefry4x64x20-AVX2 (0.40 vs 0.50 cpb).
// However, it is less universal.
static inline uint64_t get_bits_raw(void *state)
{
    Tf256VecState *obj = state;
    if (obj->intf.pos >= NWORDS * NCOPIES) {
        Tf256VecState_inc_counter(obj);
        Tf256VecState_block20(obj);
        obj->intf.pos = 0;
    }
    return obj->out[obj->intf.pos++];
}
*/


BUFGEN64_DEFINE_GET_BITS_RAW

static void *create(const CallerAPI *intf)
{
    uint64_t k[NWORDS];
    for (size_t i = 0; i < NWORDS; i++) {
        k[i] = intf->get_seed64();
    }
    Tf256Info info = Tf256Info_get(intf);
    if (info.type == TF256_THREEFRY_SCALAR) {
        intf->printf("Threefry4x64x20\n");
        Tf256State *obj = intf->malloc(sizeof(Tf256State));
        Tf256State_init(obj, k, 0);
        return obj;
    } else if (info.type == TF256_THREEFISH_SCALAR) {
        intf->printf("Threefry4x64x72 (Threefish)\n");
        Tf256State *obj = intf->malloc(sizeof(Tf256State));
        Tf256State_init(obj, k, 1);
        return obj;
    } else if (info.type == TF256_THREEFRY_AVX2) {
#ifdef THREEFRY_VEC_ENABLED
        intf->printf("Threefry4x64x20-AVX2\n");
        Tf256VecState *obj = intf->malloc(sizeof(Tf256VecState));
        Tf256VecState_init(obj, k, 0);
        return obj;
#else
        intf->printf("AVX2 is not supported on this platform\n");
        return NULL;
#endif
    } else if (info.type == TF256_THREEFISH_AVX2) {
#ifdef THREEFRY_VEC_ENABLED
        intf->printf("Threefry4x64x72 (Threefish)\n");
        Tf256VecState *obj = intf->malloc(sizeof(Tf256VecState));
        Tf256VecState_init(obj, k, 1);
        return obj;
#else
        intf->printf("AVX2 is not supported on this platform\n");
        return NULL;
#endif
    } else {
        intf->printf(
            "Unknown parameter '%s' ('threefry', 'threefish', 'threefry-avx2',\n"
            "    'threefish-avx2` are supported\n", intf->get_param());
        return NULL;
    }
}


static const char *get_prng_name(const CallerAPI *intf)
{
    return Tf256Info_get(intf).name;
}

MAKE_UINT_PRNG("Threefry4x64", run_self_test, 64, get_prng_name)
