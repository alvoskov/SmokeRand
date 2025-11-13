/**
 * @file chacha.c
 * @brief ChaCha12 pseudorandom number generator. 
 * @details ChaCha12 PRNG is a modification of ChaCha20 stream cipher with
 * reduced number of rounds. However ChaCha12 and ChaCha8 are still considered
 * cryptographically strong. ChaCha12 passes TestU01, PractRand and SmokeRand
 * test batteries and is highly recommened as a robust general purpose parallel
 * generator. It was used as one of the reference state-of-art PRNGs during
 * SmokeRand development. 
 *
 * This file contains three versions of ChaCha12:
 *
 * - `c99`:  portable version (default, around 3 cpb)
 * - `avx`:  AVX version (faster, aroudn 2-2.5 cpb)
 * - `avx2`: AVX2 version (fastest, around 1 cpb)
 *
 * Usage of AVX2 (SIMD) instructions gives about 3x speedup.
 *
 * Three are also `c99-ctr32` and `avx-ctr32` versions with 32-bit counters
 * that FAIL gap test and 64-bit birthday paradox. They MUST NOT BE USED as
 * general purpose PRNG.
 *
 * References:
 * 1. RFC 7539. ChaCha20 and Poly1305 for IETF Protocols
 *    https://datatracker.ietf.org/doc/html/rfc7539
 * 2. D.J. Bernstein. ChaCha, a variant of Salsa20. 2008.
 *    https://cr.yp.to/chacha.html
 * 3. Jean-Philippe Aumasson. Too Much Crypto // Cryptology ePrint Archive.
 *    2019. Paper 2019/1492. year = {2019}, https://eprint.iacr.org/2019/1492
 *
 * NOTE: The tests switch the generators to the ChaCha20 mode and compare
 * output with RFC 7539 reference values. All three versions should give
 * bit-to-bit identical output.
 *
 * WARNING! This program is designed as a general purpose high quality PRNG
 * for simulations and statistical testing. IT IS NOT DESIGNED FOR ENCRYPTION,
 * KEYS/NONCES GENERATION AND OTHER CRYPTOGRAPHICAL APPLICATION!
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#ifdef  __AVX__
#define CHACHA_VECTOR_INTR
#include "smokerand/x86exts.h"
#endif

#ifdef __AVX2__
#define CHACHA_VECTOR_AVX2
#endif

enum {
    GEN_NROUNDS = 12, ///< Number of rounds for ChaCha12
    GEN_NROUNDS_FULL = 20 ///< Number of rounds for ChaCha20
};


/**
 * @brief Contains the ChaCha state.
 * @details The next memory layout in 1D array is used:
 * 
 *     | 0   1  2  3 |
 *     | 4   5  6  7 |
 *     | 8   9 10 11 |
 *     | 12 13 14 15 |
 */
typedef struct {
    uint32_t x[16];   ///< Working state
    uint32_t out[16]; ///< Output state
    size_t ncycles;   ///< Number of rounds / 2
    size_t pos;
} ChaChaState;


/**
 * @brief Contains the state for four parallel ChaCha states.
 * @details The next memory layout in 1D array is used:
 * 
 *     | 0   1  2  3 |  4  5  6  7 |
 *     | 8   9 10 11 | 12 13 14 15 |
 *     | 16 17 18 19 | 20 21 22 23 |
 *     | 24 25 26 27 | 28 29 30 31 |
 *
 *     | 32 34 35 36 | 37 38 39 40 |
 *     | 40 41 42 43 | 44 45 46 47 |
 *     | 48 49 50 51 | 52 53 54 55 |
 *     | 56 57 58 59 | 60 61 62 63 |
 *
 * Block layout for one ChaCha PRNG
 *
 *     | const const const const |
 *     | key   key   key   key   |
 *     | key   key   key   key   |
 *     | ctr   ctr   nonce nonce |
 * 
 */
typedef struct {
    uint32_t x[64];    ///< Working state
    uint32_t out[64];  ///< Output state
    size_t ncycles;    ///< Number of rounds / 2
    size_t pos;
} ChaChaVecState;


/**
 * @brief A cross-platform increment of 64-bit counter inside 32-bit
 * array (friendly to x86-64 little endian)
 */
static inline void add_to_ctr32(uint32_t *ctr, uint32_t inc)
{
    uint64_t ctr64 = (uint64_t) ctr[0] + ((uint64_t) ctr[1] << 32);
    ctr64 += inc;
    ctr[0] = (uint32_t) ctr64;
    ctr[1] = (uint32_t) (ctr64 >> 32);
}

/////////////////////////////////////////
///// Cross-platform scalar version /////
/////////////////////////////////////////

/**
 * @brief Qround (cross-platform scalar implementation)
 */
static inline void qround(uint32_t *x, size_t ai, size_t bi, size_t ci, size_t di)
{
    x[ai] += x[bi]; x[di] ^= x[ai]; x[di] = rotl32(x[di], 16);
    x[ci] += x[di]; x[bi] ^= x[ci]; x[bi] = rotl32(x[bi], 12);
    x[ai] += x[bi]; x[di] ^= x[ai]; x[di] = rotl32(x[di], 8);
    x[ci] += x[di]; x[bi] ^= x[ci]; x[bi] = rotl32(x[bi], 7);
}


/**
 * @brief Implementation of ChaCha rounds for a 512-bit block.
 * The function is exported for debugging purposes.
 * @details The scheme of rounds are:
 *
 *     | x . . . |    | . x . . |    | . . x . |    | . . . x |
 *     | x . . . | => | . x . . | => | . . x . | => | . . . x |
 *     | x . . . |    | . x . . |    | . . x . |    | . . . x |
 *     | x . . . |    | . x . . |    | . . x . |    | . . . x |
 * 
 *     | x . . . |    | . x . . |    | . . x . |    | . . . x |
 *     | . x . . | => | . . x . | => | . . . x | => | x . . . |
 *     | . . x . |    | . . . x |    | x . . . |    | . x . . |
 *     | . . . x |    | x . . . |    | . x . . |    | . . x . |
 */
void ChaCha_block_c99(ChaChaState *obj)
{
    for (size_t k = 0; k < 16; k++) {
        obj->out[k] = obj->x[k];
    }

    for (size_t k = 0; k < obj->ncycles; k++) {
        // Vertical qrounds
        qround(obj->out, 0, 4, 8,12);
        qround(obj->out, 1, 5, 9,13);
        qround(obj->out, 2, 6,10,14);
        qround(obj->out, 3, 7,11,15);
        // Diagonal qrounds
        qround(obj->out, 0, 5,10,15);
        qround(obj->out, 1, 6,11,12);
        qround(obj->out, 2, 7, 8,13);
        qround(obj->out, 3, 4, 9,14);
    }
    for (size_t i = 0; i < 16; i++) {
        obj->out[i] += obj->x[i];
    }
}


/**
 * @brief Increase the value of 128-bit PRNG counter.
 */
static inline void ChaCha_inc_counter(ChaChaState *obj)
{
    if (++obj->x[12] == 0) ++obj->x[13];
}
/**
 * @brief Increase the value of 32-bit PRNG counter.
 * ONLY FOR DEBUGGING PURPOSES! Such PRNG will fail gap test
 * and 64-bit birthday paradox test!
 */
static inline void ChaCha_inc_counter32(ChaChaState *obj)
{
    obj->x[12]++;
}


/**
 * @brief Initialize the state of ChaCha CSPRNG.
 * The function is exported for debugging purposes.
 * @param obj     The state to be initialized.
 * @param nrounds Number of rounds (8, 12, 20).
 * @param seed    Pointer to array of 8 uint32_t values (seeds).
 * @memberof ChaChaState
 */
void ChaCha_init(ChaChaState *obj, size_t nrounds, const uint32_t *seed)
{
    // Constants: the upper row of the matrix
    obj->x[0] = 0x61707865; obj->x[1] = 0x3320646e;
    obj->x[2] = 0x79622d32; obj->x[3] = 0x6b206574;
    // Rows 1-2: seed (key)
    for (size_t i = 0; i < 8; i++) {
        obj->x[i + 4] = seed[i];
    }
    // Row 3: counter and nonce
    for (size_t i = 12; i <= 15; i++) {
        obj->x[i] = 0;
    }
    // Prepare state    
    obj->ncycles = nrounds / 2; // Number of rounds => Number of cycles
    ChaCha_block_c99(obj);    
    obj->pos = 0; // Output counter
}


static inline uint64_t get_bits_c99_raw(void *state)
{
    ChaChaState *obj = state;
    if (obj->pos >= 16) {
        ChaCha_inc_counter(obj);
        ChaCha_block_c99(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(c99)

static inline uint64_t get_bits_c99ctr32_raw(void *state)
{
    ChaChaState *obj = state;
    if (obj->pos >= 16) {
        ChaCha_inc_counter32(obj);
        ChaCha_block_c99(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(c99ctr32)


/**
 * @brief Print the 4x4 matrix of uint32_t from the ChaCha PRNG state.
 * @param x Pointer to the matrix (C-style)
 */
static void print_mat16(const CallerAPI *intf, const uint32_t *x)
{
    for (size_t i = 0; i < 16; i++) {
        intf->printf("%10.8X ", x[i]);
        if ((i + 1) % 4 == 0)
            intf->printf("\n");
    }
}


/**
 * @brief Internal self-test. Based on reference values from RFC 7359.
 */
static int run_self_test_scalar(const CallerAPI *intf, void (*blockfunc)(ChaChaState *obj))
{
    static const uint32_t x_init[] = { // Input values
        0x03020100,  0x07060504,  0x0b0a0908,  0x0f0e0d0c,
        0x13121110,  0x17161514,  0x1b1a1918,  0x1f1e1d1c,
        0x00000001,  0x09000000,  0x4a000000,  0x00000000
    };
    static const uint32_t out_final[] = { // Refernce values from RFC 7359
       0xe4e7f110,  0x15593bd1,  0x1fdd0f50,  0xc47120a3,
       0xc7f4d1c7,  0x0368c033,  0x9aaa2204,  0x4e6cd4c3,
       0x466482d2,  0x09aa9f07,  0x05d7c214,  0xa2028bd9,
       0xd19c12b5,  0xb94e16de,  0xe883d0cb,  0x4e3c50a2
    };
    ChaChaState obj;
    ChaCha_init(&obj, GEN_NROUNDS_FULL, x_init); // 20 rounds
    for (size_t i = 0; i < 12; i++) {
        obj.x[i + 4] = x_init[i];
    }
    intf->printf("Input:\n"); print_mat16(intf, obj.x);
    blockfunc(&obj);
    intf->printf("Output (real):\n"); print_mat16(intf, obj.out);
    intf->printf("Output (reference):\n"); print_mat16(intf, out_final);
    for (size_t i = 0; i < 16; i++) {
        if (out_final[i] != obj.out[i]) {
            intf->printf("TEST FAILED!\n");
            return 0;
        }        
    }
    intf->printf("Success.\n");
    return 1;
}


static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    ChaChaState *obj = intf->malloc(sizeof(ChaChaState));
    uint32_t seeds[8];
    for (size_t i = 0; i < 4; i++) {
        seed64_to_2x32(intf, &seeds[2*i], &seeds[2*i + 1]);
    }
    ChaCha_init(obj, GEN_NROUNDS, seeds);
    (void) gi;
    return obj;
}


//////////////////////////////////////////////////////////
///// AVX version that works with one copy of ChaCha /////
//////////////////////////////////////////////////////////

#ifdef CHACHA_VECTOR_INTR

static inline __m128i mm_roti_epi32_def(__m128i in, int r)
{
    return _mm_or_si128(_mm_slli_epi32(in, r), _mm_srli_epi32(in, 32 - r));
}

static inline __m128i mm_rot16_epi32_def(__m128i in)
{
    return _mm_shuffle_epi8(in,
        _mm_set_epi8(13,12,15,14,  9,8,11,10,  5,4,7,6,  1,0,3,2));
}

static inline __m128i mm_rot8_epi32_def(__m128i in)
{
    return _mm_shuffle_epi8(in,
        _mm_set_epi8(14,13,12,15,  10,9,8,11,  6,5,4,7,  2,1,0,3));
}

/**
 * @brief Vertical qround (hardware vectorization for x86-64)
 */
static inline void qround_avx(__m128i *a, __m128i *b, __m128i *c, __m128i *d)
{
    *a = _mm_add_epi32(*a, *b); *d = _mm_xor_si128(*d, *a); *d = mm_rot16_epi32_def(*d);
    *c = _mm_add_epi32(*c, *d); *b = _mm_xor_si128(*b, *c); *b = mm_roti_epi32_def(*b, 12);
    *a = _mm_add_epi32(*a, *b); *d = _mm_xor_si128(*d, *a); *d = mm_rot8_epi32_def(*d);
    *c = _mm_add_epi32(*c, *d); *b = _mm_xor_si128(*b, *c); *b = mm_roti_epi32_def(*b, 7);
}
#endif


void ChaCha_block_avx(ChaChaState *obj)
{
#ifdef CHACHA_VECTOR_INTR
    const __m128i *w128x = (__m128i *) (void *) (&obj->x[0]);
    __m128i *w128o = (__m128i *) (void *) (&obj->out[0]);
    __m128i a = _mm_loadu_si128(&w128x[0]); // words 0..3
    __m128i b = _mm_loadu_si128(&w128x[1]); // words 4..7
    __m128i c = _mm_loadu_si128(&w128x[2]); // words 8..11
    __m128i d = _mm_loadu_si128(&w128x[3]); // words 12..15
    __m128i ax = a, bx = b, cx = c, dx = d;
    for (size_t k = 0; k < obj->ncycles; k++) {
        // Vertical qround
        qround_avx(&a, &b, &c, &d);
        // Diagonal qround; the original vector is [3 2 1 0]
        b = _mm_shuffle_epi32(b, 0x39); // [0 3 2 1] -> 3 (or <- 1)
        c = _mm_shuffle_epi32(c, 0x4E); // [1 0 3 2] -> 2 (or <- 2)
        d = _mm_shuffle_epi32(d, 0x93); // [2 1 0 3] -> 1 (or <- 3)
        qround_avx(&a, &b, &c, &d);
        b = _mm_shuffle_epi32(b, 0x93);
        c = _mm_shuffle_epi32(c, 0x4E);
        d = _mm_shuffle_epi32(d, 0x39);
    }
    a = _mm_add_epi32(a, ax);
    b = _mm_add_epi32(b, bx);
    c = _mm_add_epi32(c, cx);
    d = _mm_add_epi32(d, dx);

    _mm_storeu_si128(&w128o[0], a);
    _mm_storeu_si128(&w128o[1], b);
    _mm_storeu_si128(&w128o[2], c);
    _mm_storeu_si128(&w128o[3], d);
#else
    (void) obj;
#endif
}



static inline uint64_t get_bits_avx_raw(void *state)
{
    ChaChaState *obj = state;
    if (obj->pos >= 16) {
        ChaCha_inc_counter(obj);
        ChaCha_block_avx(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(avx)


static inline uint64_t get_bits_avxctr32_raw(void *state)
{
    ChaChaState *obj = state;
    if (obj->pos >= 16) {
        ChaCha_inc_counter32(obj);
        ChaCha_block_avx(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(avxctr32)


/////////////////////////////////////////////////////
///// AVX2 (vectorized) version implementation //////
/////////////////////////////////////////////////////


/**
 * @brief Increase the value of 64-bit PRNG counter.
 */
static inline void ChaChaVec_inc_counter(ChaChaVecState *obj)
{
    add_to_ctr32(&obj->x[24], 4); // words 24,25
    add_to_ctr32(&obj->x[28], 4); // words 28,29
    add_to_ctr32(&obj->x[56], 4); // words 56,57
    add_to_ctr32(&obj->x[60], 4); // words 60,61
}


#ifdef CHACHA_VECTOR_AVX2
static inline __m256i mm256_roti_epi32_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi32(in, r), _mm256_srli_epi32(in, 32 - r));
}

static inline __m256i mm256_rot16_epi32_def(__m256i in)
{
    return _mm256_shuffle_epi8(in,
        _mm256_set_epi8(
            29,28,31,30,  25,24,27,26, 21,20,23,22, 17,16,19,18,
            13,12,15,14,  9,8,11,10,   5,4,7,6,     1,0,3,2));
}

static inline __m256i mm256_rot8_epi32_def(__m256i in)
{
    return _mm256_shuffle_epi8(in,
        _mm256_set_epi8(
            30,29,28,31,  26,25,24,27, 22,21,20,23, 18,17,16,19,
            14,13,12,15,  10,9,8,11,   6,5,4,7,     2,1,0,3));
}


/**
 * @brief Vertical qround (hardware vectorization for x86-64)
 */
static inline void qround_avx2(__m256i *a, __m256i *b, __m256i *c, __m256i *d)
{
    *a = _mm256_add_epi32(*a, *b);
    *d = _mm256_xor_si256(*d, *a);
    *d = mm256_rot16_epi32_def(*d);

    *c = _mm256_add_epi32(*c, *d);
    *b = _mm256_xor_si256(*b, *c);
    *b = mm256_roti_epi32_def(*b, 12);

    *a = _mm256_add_epi32(*a, *b);
    *d = _mm256_xor_si256(*d, *a);
    *d = mm256_rot8_epi32_def(*d);

    *c = _mm256_add_epi32(*c, *d);
    *b = _mm256_xor_si256(*b, *c);
    *b = mm256_roti_epi32_def(*b, 7);
}
#endif

/**
 * @brief Implementation of ChaCha rounds for a 512-bit block.
 * The function is exported for debugging purposes.
 * @details The scheme of rounds are:
 *
 *     | x . . . |    | . x . . |    | . . x . |    | . . . x |
 *     | x . . . | => | . x . . | => | . . x . | => | . . . x |
 *     | x . . . |    | . x . . |    | . . x . |    | . . . x |
 *     | x . . . |    | . x . . |    | . . x . |    | . . . x |
 * 
 *     | x . . . |    | . x . . |    | . . x . |    | . . . x |
 *     | . x . . | => | . . x . | => | . . . x | => | x . . . |
 *     | . . x . |    | . . . x |    | x . . . |    | . x . . |
 *     | . . . x |    | x . . . |    | . x . . |    | . . x . |
 */
void EXPORT ChaChaVec_block(ChaChaVecState *obj)
{
#ifdef CHACHA_VECTOR_AVX2
    const __m256i *w256x = (__m256i *) (void *) (&obj->x[0]);
    __m128i *w128o = (__m128i *) (void *) (&obj->out[0]);
    __m256i a = _mm256_loadu_si256(&w256x[0]); // words 0..7
    __m256i b = _mm256_loadu_si256(&w256x[1]); // words 8..15
    __m256i c = _mm256_loadu_si256(&w256x[2]); // words 16..23
    __m256i d = _mm256_loadu_si256(&w256x[3]); // words 24..31

    __m256i a2 = _mm256_loadu_si256(&w256x[4]); // words 32..39
    __m256i b2 = _mm256_loadu_si256(&w256x[5]); // words 40..47
    __m256i c2 = _mm256_loadu_si256(&w256x[6]); // words 48..55
    __m256i d2 = _mm256_loadu_si256(&w256x[7]); // words 56..63

    __m256i ax = a, bx = b, cx = c, dx = d;
    __m256i ax2 = a2, bx2 = b2, cx2 = c2, dx2 = d2;
    for (size_t k = 0; k < obj->ncycles; k++) {
        // Generators 0-1
        /* Vertical qround */
        qround_avx2(&a, &b, &c, &d);
        // Diagonal qround; the original vector is [3 2 1 0]
        b = _mm256_shuffle_epi32(b, 0x39); // [0 3 2 1] -> 3 (or <- 1)
        c = _mm256_shuffle_epi32(c, 0x4E); // [1 0 3 2] -> 2 (or <- 2)
        d = _mm256_shuffle_epi32(d, 0x93); // [2 1 0 3] -> 1 (or <- 3)
        qround_avx2(&a, &b, &c, &d);
        b = _mm256_shuffle_epi32(b, 0x93);
        c = _mm256_shuffle_epi32(c, 0x4E);
        d = _mm256_shuffle_epi32(d, 0x39);
        // Generators 2-3
        // Vertical qround
        qround_avx2(&a2, &b2, &c2, &d2);
        // Diagonal qround; the original vector is [3 2 1 0]
        b2 = _mm256_shuffle_epi32(b2, 0x39); // [0 3 2 1] -> 3 (or <- 1)
        c2 = _mm256_shuffle_epi32(c2, 0x4E); // [1 0 3 2] -> 2 (or <- 2)
        d2 = _mm256_shuffle_epi32(d2, 0x93); // [2 1 0 3] -> 1 (or <- 3)
        qround_avx2(&a2, &b2, &c2, &d2);
        b2 = _mm256_shuffle_epi32(b2, 0x93);
        c2 = _mm256_shuffle_epi32(c2, 0x4E);
        d2 = _mm256_shuffle_epi32(d2, 0x39);
    }
    a = _mm256_add_epi32(a, ax);
    b = _mm256_add_epi32(b, bx);
    c = _mm256_add_epi32(c, cx);
    d = _mm256_add_epi32(d, dx);

    a2 = _mm256_add_epi32(a2, ax2);
    b2 = _mm256_add_epi32(b2, bx2);
    c2 = _mm256_add_epi32(c2, cx2);
    d2 = _mm256_add_epi32(d2, dx2);

    // Generator 0
    _mm_storeu_si128(&w128o[0],  _mm256_castsi256_si128(a));
    _mm_storeu_si128(&w128o[1],  _mm256_castsi256_si128(b));
    _mm_storeu_si128(&w128o[2],  _mm256_castsi256_si128(c));
    _mm_storeu_si128(&w128o[3],  _mm256_castsi256_si128(d));

    // Generator 1
    _mm_storeu_si128(&w128o[4],  _mm256_extracti128_si256(a, 1));
    _mm_storeu_si128(&w128o[5],  _mm256_extracti128_si256(b, 1));
    _mm_storeu_si128(&w128o[6],  _mm256_extracti128_si256(c, 1));
    _mm_storeu_si128(&w128o[7],  _mm256_extracti128_si256(d, 1));

    // Generator 2
    _mm_storeu_si128(&w128o[8],  _mm256_castsi256_si128(a2));
    _mm_storeu_si128(&w128o[9],  _mm256_castsi256_si128(b2));
    _mm_storeu_si128(&w128o[10], _mm256_castsi256_si128(c2));
    _mm_storeu_si128(&w128o[11], _mm256_castsi256_si128(d2));

    // Generator 3
    _mm_storeu_si128(&w128o[12], _mm256_extracti128_si256(a2, 1));
    _mm_storeu_si128(&w128o[13], _mm256_extracti128_si256(b2, 1));
    _mm_storeu_si128(&w128o[14], _mm256_extracti128_si256(c2, 1));
    _mm_storeu_si128(&w128o[15], _mm256_extracti128_si256(d2, 1));
#else
    (void) obj;
#endif
}


/**
 * @brief Initialize the state of ChaCha CSPRNG.
 * The function is exported for debugging purposes.
 * @param obj     The state to be initialized.
 * @param nrounds Number of rounds (8, 12, 20).
 * @param seed    Pointer to array of 8 uint32_t values (seeds).
 */
void EXPORT ChaChaVec_init(ChaChaVecState *obj, size_t nrounds, const uint32_t *seed)
{
    // Fill input and output state with zeros
    for (size_t i = 0; i < 64; i++) {
        obj->x[i] = 0;
        obj->out[i] = 0;
    }
    // Constants: the upper row of the matrix
    obj->x[0] = 0x61707865; obj->x[1] = 0x3320646e;
    obj->x[2] = 0x79622d32; obj->x[3] = 0x6b206574;
    for (size_t i = 0; i < 4; i++) { // From gen.1 to gen.0
        obj->x[i + 4] = obj->x[i];
    }
    // Rows 1-2: seed (key)
    // | 8   9 10 11 | 12 13 14 15 | <- gen.0-1
    // | 16 17 18 19 | 20 21 22 23 |
    // | 40 41 42 43 | 44 45 46 47 | <- gen.2-3
    // | 48 49 50 51 | 52 53 54 55 |
    for (size_t i = 0; i < 4; i++) {
        obj->x[i + 8]  = seed[i];     obj->x[i + 12] = seed[i];
        obj->x[i + 16] = seed[i + 4]; obj->x[i + 20] = seed[i + 4];
    }
    // Copy constant and key from gen.0-1 to gen.2-3
    for (size_t i = 0; i < 24; i++) {
        obj->x[i + 32] = obj->x[i];
    }
    // Row 3: counter and nonce
    obj->x[28] = 1; obj->x[29] = 0; // words 28,29
    obj->x[56] = 2; obj->x[57] = 0; // words 56,57
    obj->x[60] = 3; obj->x[61] = 0; // words 60,61
    // Prepare state
    obj->ncycles = nrounds / 2; // Number of rounds => Number of cycles
    ChaChaVec_block(obj);
    obj->pos = 0;
}


static uint64_t get_bits_vector_raw(void *state)
{
    ChaChaVecState *obj = state;
    if (obj->pos >= 64) {
        ChaChaVec_inc_counter(obj);
        ChaChaVec_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(vector)

static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef CHACHA_VECTOR_AVX2
    ChaChaVecState *obj = intf->malloc(sizeof(ChaChaVecState));
    uint32_t seeds[8];
    for (size_t i = 0; i < 4; i++) {
        seed64_to_2x32(intf, &seeds[2*i], &seeds[2*i + 1]);
    }
    ChaChaVec_init(obj, GEN_NROUNDS, seeds);
    (void) gi;
    return obj;
#else
    (void) intf; (void) gi;
    return NULL;
#endif
}

/**
 * @brief Print the ncols matrix of uint32_t from the ChaCha PRNG state.
 * @param x      Pointer to the matrix (C-style)
 * @param ncols  Number of columns.                       n
 * @param nelem  Number of elements.
 */
void print_matx(const CallerAPI *intf, const uint32_t *x, size_t ncols, size_t nelem)
{
    for (size_t i = 0; i < nelem; i++) {
        intf->printf("%10.8X ", x[i]);
        if ((i + 1) % ncols == 0)
            intf->printf("\n");
    }
}

/**
 * @brief Internal self-test. Based on reference values from RFC 7359.
 */
int run_self_test_vector(const CallerAPI *intf)
{
    static const uint32_t x_init[] = { // Input values
        0x03020100,  0x07060504,  0x0b0a0908,  0x0f0e0d0c,
        0x13121110,  0x17161514,  0x1b1a1918,  0x1f1e1d1c,
        0x00000001,  0x09000000,  0x4a000000,  0x00000000
    };    
    static const uint32_t out_final[] = { // Reference values from RFC 7359
       0xe4e7f110,  0x15593bd1,  0x1fdd0f50,  0xc47120a3,
       0xc7f4d1c7,  0x0368c033,  0x9aaa2204,  0x4e6cd4c3,
       0x466482d2,  0x09aa9f07,  0x05d7c214,  0xa2028bd9,
       0xd19c12b5,  0xb94e16de,  0xe883d0cb,  0x4e3c50a2
    };
    ChaChaVecState obj;
    ChaChaVec_init(&obj, GEN_NROUNDS_FULL, x_init);
    // ChaCha states are loaded in permuted form inside its state
    for (size_t i = 0; i < 4; i++) {
        obj.x[i + 8]  = obj.x[i + 12] = x_init[i]; // Row 2
        obj.x[i + 16] = obj.x[i + 20] = x_init[i + 4]; // Row 3
        obj.x[i + 24] = obj.x[i + 28] = x_init[i + 8]; // Row 4
    }
    for (size_t i = 0; i < 32; i++) {
        obj.x[i + 32] = obj.x[i];
    }
    intf->printf("Input:\n"); print_matx(intf, obj.x, 4, 64);
    ChaChaVec_block(&obj);
    intf->printf("Output (real):\n"); print_matx(intf, obj.out, 4, 64);
    intf->printf("Output (reference):\n"); print_matx(intf, out_final, 4, 16);
    for (size_t i = 0; i < 64; i++) {
        if (out_final[i % 16] != obj.out[i]) {
            intf->printf("TEST FAILED!\n");
            return 0;
        }        
    }
    intf->printf("Success.\n");
    return 1;
}



//////////////////////
///// Interfaces /////
//////////////////////

static void *create(const CallerAPI *intf)
{
    intf->printf("'%s' not implemented\n", intf->get_param());
    return NULL;
}

int run_self_test(const CallerAPI *intf)
{
    static const unsigned long nskipped = 1UL << 10;
    static const size_t nsamples = 8192;
    static const uint32_t key[] = { // Input values
        0x03020100,  0x07060504,  0x0b0a0908,  0x0f0e0d0c,
        0x13121110,  0x17161514,  0x1b1a1918,  0x1f1e1d1c
    };

    int is_ok = 1;
    intf->printf("----- ChaCha: c99 version -----\n");
    is_ok = is_ok & run_self_test_scalar(intf, ChaCha_block_c99);
#ifdef CHACHA_VECTOR_INTR
    intf->printf("----- ChaCha: AVX version -----\n");
    is_ok = is_ok & run_self_test_scalar(intf, ChaCha_block_avx);
#endif
#ifdef CHACHA_VECTOR_AVX2
    intf->printf("----- ChaCha: AVX2 version -----\n");
    is_ok = is_ok & run_self_test_vector(intf);
#endif


#if defined(CHACHA_VECTOR_INTR) || defined(CHACHA_VECTOR_AVX2)
    intf->printf("Comparing with portable version...\n");
    uint32_t *c99_out = intf->malloc(nsamples * sizeof(uint32_t));
    ChaChaState *obj_c99 = intf->malloc(sizeof(ChaChaState));
    ChaCha_init(obj_c99, GEN_NROUNDS_FULL, key); // 20 rounds
    for (unsigned long i = 0; i < nskipped; i++) {
       (void) get_bits_c99_raw(obj_c99);
    }
    for (size_t i = 0; i < nsamples; i++) {
        c99_out[i] = (uint32_t) get_bits_c99_raw(obj_c99);
    }
#endif

#ifdef CHACHA_VECTOR_INTR
    ChaCha_init(obj_c99, GEN_NROUNDS_FULL, key); // 20 rounds
    for (unsigned long i = 0; i < nskipped; i++) {
        (void) get_bits_avx_raw(obj_c99);
    }
    for (size_t i = 0; i < nsamples; i++) {
        const uint32_t out = (uint32_t) get_bits_avx_raw(obj_c99);
        if (out != c99_out[i]) {
            intf->printf("AVX version output is corrupted\n");
            is_ok = 0;
            break;
        }
    }
    if (is_ok) {
        intf->printf("AVX version output is ok\n");
    }
#endif

#ifdef CHACHA_VECTOR_AVX2
    ChaChaVecState *obj_avx2 = intf->malloc(sizeof(ChaChaVecState));
    ChaChaVec_init(obj_avx2, GEN_NROUNDS_FULL, key); // 20 rounds
    for (unsigned long i = 0; i < (1UL << 10); i++) {
        (void) get_bits_vector_raw(obj_avx2);
    }
    for (unsigned int i = 0; i < 128; i++) {
        const uint32_t out = (uint32_t) get_bits_vector_raw(obj_avx2);
        if (out != c99_out[i]) {
            intf->printf("AVX2 version output is corrupted\n");
            is_ok = 0;
            break;
        }
    }
    if (is_ok) {
        intf->printf("AVX2 version output is ok\n");
    }
    intf->free(obj_avx2);
#endif

    // Deallocating memory used for comparision of different version
#if defined(CHACHA_VECTOR_INTR) || defined(CHACHA_VECTOR_AVX2)
    intf->free(obj_c99);
    intf->free(c99_out);
#else
    (void) nskipped; (void) nsamples;
#endif
    // Success
    return is_ok;
}


static const char description[] =
"ChaCha block cipher based PRNGs\n"
"param values are supported:\n"
"  c99       - portable ChaCha12 version (default, slower)\n"
"  avx       - AVX ChaCha12 version (faster)\n"
"  avx2      - AVX2 ChaCha12 version (fastest)\n"
"  c99-ctr32 - c99 variant with 32-bit counter (WILL FAIL SOME TESTS!)\n"
"  avx-ctr32 - avx variant with 32-bit counter (WILL FAIL SOME TESTS!)\n";



int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 32;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "c99") || !intf->strcmp(param, "")) {
        gi->name = "ChaCha:c99";
        gi->create = create_scalar;
        gi->get_bits = get_bits_c99;
        gi->get_sum = get_sum_c99;
    } else if (!intf->strcmp(param, "c99-ctr32")) {
        gi->name = "ChaCha:c99-ctr32";
        gi->create = create_scalar;
        gi->get_bits = get_bits_c99ctr32;
        gi->get_sum = get_sum_c99ctr32;
    } else if (!intf->strcmp(param, "avx")) {
        gi->name = "ChaCha:avx";
#ifdef CHACHA_VECTOR_INTR
        gi->create = create_scalar;
#endif
        gi->get_bits = get_bits_avx;
        gi->get_sum = get_sum_avx;
    } else if (!intf->strcmp(param, "avx-ctr32")) {
        gi->name = "ChaCha:avx-ctr32";
#ifdef CHACHA_VECTOR_INTR
        gi->create = create_scalar;
#endif
        gi->get_bits = get_bits_avxctr32;
        gi->get_sum = get_sum_avxctr32;
    } else if (!intf->strcmp(param, "avx2")) {
        gi->name = "ChaCha:avx2";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else {
        gi->name = "ChaCha:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
