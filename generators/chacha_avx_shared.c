/**
 * @file chacha_avx_shared.c
 * @brief ChaCha12 pseudorandom number generator (AVX version). 
 * @details SIMD version of ChaCha12 generator that uses 256-bit YMM CPU
 * registers. Usage of AVX instructions gives about 3x speedup.
 * See chacha_shared.h for the detailed comments about the generator.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

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
    uint32_t x[64]; ///< Working state
    uint32_t out[64]; ///< Output state
    size_t ncycles; ///< Number of rounds / 2
    size_t pos;
} ChaChaAVXState;


#include <x86intrin.h>
//#include <emmintrin.h>
//#include <tmmintrin.h>
//#include <xopintrin.h>

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
static inline void mm_qround_vert(__m256i *a, __m256i *b, __m256i *c, __m256i *d)
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

/**
 * @brief Increase the value of 64-bit PRNG counter.
 */
static inline void ChaChaAVX_inc_counter(ChaChaAVXState *obj)
{
    // 128-bit counters
    uint64_t *cnt1 = (uint64_t *) &obj->x[24];
    uint64_t *cnt2 = (uint64_t *) &obj->x[28];
    uint64_t *cnt3 = (uint64_t *) &obj->x[56];
    uint64_t *cnt4 = (uint64_t *) &obj->x[60];

    *cnt1 += 4;
    *cnt2 += 4;
    *cnt3 += 4;
    *cnt4 += 4;
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
void EXPORT ChaChaAVX_block(ChaChaAVXState *obj)
{
    __m256i a = _mm256_loadu_si256((__m256i *) obj->x);
    __m256i b = _mm256_loadu_si256((__m256i *) (obj->x + 8));
    __m256i c = _mm256_loadu_si256((__m256i *) (obj->x + 16));
    __m256i d = _mm256_loadu_si256((__m256i *) (obj->x + 24));

    __m256i a2 = _mm256_loadu_si256((__m256i *) (obj->x + 32));
    __m256i b2 = _mm256_loadu_si256((__m256i *) (obj->x + 40));
    __m256i c2 = _mm256_loadu_si256((__m256i *) (obj->x + 48));
    __m256i d2 = _mm256_loadu_si256((__m256i *) (obj->x + 56));

    __m256i ax = a, bx = b, cx = c, dx = d;
    __m256i ax2 = a2, bx2 = b2, cx2 = c2, dx2 = d2;
    for (size_t k = 0; k < obj->ncycles; k++) {
        // Generators 0-1
        /* Vertical qround */
        mm_qround_vert(&a, &b, &c, &d);
        /* Diagonal qround; the original vector is [3 2 1 0] */
        b = _mm256_shuffle_epi32(b, 0x39); // [0 3 2 1] -> 3 (or <- 1)
        c = _mm256_shuffle_epi32(c, 0x4E); // [1 0 3 2] -> 2 (or <- 2)
        d = _mm256_shuffle_epi32(d, 0x93); // [2 1 0 3] -> 1 (or <- 3)
        mm_qround_vert(&a, &b, &c, &d);
        b = _mm256_shuffle_epi32(b, 0x93);
        c = _mm256_shuffle_epi32(c, 0x4E);
        d = _mm256_shuffle_epi32(d, 0x39);
        // Generators 2-3
        /* Vertical qround */
        mm_qround_vert(&a2, &b2, &c2, &d2);
        /* Diagonal qround; the original vector is [3 2 1 0] */
        b2 = _mm256_shuffle_epi32(b2, 0x39); // [0 3 2 1] -> 3 (or <- 1)
        c2 = _mm256_shuffle_epi32(c2, 0x4E); // [1 0 3 2] -> 2 (or <- 2)
        d2 = _mm256_shuffle_epi32(d2, 0x93); // [2 1 0 3] -> 1 (or <- 3)
        mm_qround_vert(&a2, &b2, &c2, &d2);
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


    _mm256_storeu_si256((__m256i *) obj->out, a);
    _mm256_storeu_si256((__m256i *) (obj->out + 8), b);
    _mm256_storeu_si256((__m256i *) (obj->out + 16), c);
    _mm256_storeu_si256((__m256i *) (obj->out + 24), d);
    _mm256_storeu_si256((__m256i *) (obj->out + 32), a2);
    _mm256_storeu_si256((__m256i *) (obj->out + 40), b2);
    _mm256_storeu_si256((__m256i *) (obj->out + 48), c2);
    _mm256_storeu_si256((__m256i *) (obj->out + 56), d2);
}

/**
 * @brief Initialize the state of ChaCha CSPRNG.
 * The function is exported for debugging purposes.
 * @param obj     The state to be initialized.
 * @param nrounds Number of rounds (8, 12, 20).
 * @param seed    Pointer to array of 8 uint32_t values (seeds).
 */
void EXPORT ChaChaAVX_init(ChaChaAVXState *obj, size_t nrounds, const uint32_t *seed)
{
    // Fill input and output state with zeros
    for (size_t i = 0; i < 64; i++) {
        obj->x[i] = 0;
        obj->out[i] = 0;
    }
    /* Constants: the upper row of the matrix */
    obj->x[0] = 0x61707865; obj->x[1] = 0x3320646e;
    obj->x[2] = 0x79622d32; obj->x[3] = 0x6b206574;
    for (size_t i = 0; i < 4; i++) { // From gen.1 to gen.0
        obj->x[i + 4] = obj->x[i];
    }
    /* Rows 1-2: seed (key) */
    // | 8   9 10 11 | 12 13 14 15 | <- gen.0-1
    // | 16 17 18 19 | 20 21 22 23 |
    // | 40 41 42 43 | 44 45 46 47 | <- gen.2-3
    // | 48 49 50 51 | 52 53 54 55 |
    //for (size_t i = 0; i < 24; i++) {
    //    obj->x[i + 8] = 0xFF;
    //}
    for (size_t i = 0; i < 4; i++) {
        obj->x[i + 8]  = seed[i];     obj->x[i + 12] = seed[i];
        obj->x[i + 16] = seed[i + 4]; obj->x[i + 20] = seed[i + 4];
    }


    // Copy constant and key from gen.0-1 to gen.2-3
    for (size_t i = 0; i < 24; i++) {
        obj->x[i + 32] = obj->x[i];
    }

    // Row 3: counter and nonce
    {
        uint64_t *cnt = (uint64_t *) &obj->x[28]; cnt[0] = 1;
        cnt = (uint64_t *) &obj->x[56]; cnt[0] = 2;
        cnt = (uint64_t *) &obj->x[60]; cnt[0] = 3;
    }
    ChaChaAVX_inc_counter(obj);
    /* Number of rounds => Number of cycles */
    obj->ncycles = nrounds / 2;
    /* Output counter */
    obj->pos = 64;
}

static uint64_t get_bits_raw(void *state)
{
    ChaChaAVXState *obj = state;
    if (obj->pos >= 64) {
        ChaChaAVX_inc_counter(obj);
        ChaChaAVX_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    ChaChaAVXState *obj = intf->malloc(sizeof(ChaChaAVXState));
    uint32_t seeds[8];
    for (size_t i = 0; i < 4; i++) {
        uint64_t s = intf->get_seed64();
        seeds[2*i] = s & 0xFFFFFFF;
        seeds[2*i + 1] = s >> 32;
    }

    ChaChaAVX_init(obj, 12, seeds);
    return (void *) obj;
}

/**
 * @brief Print the ncols matrix of uint32_t from the ChaCha PRNG state.
 * @param x      Pointer to the matrix (C-style)
 * @param ncols  Number of columns.
 * @param nelem  Number of elements.
 */
static void print_matx(const CallerAPI *intf, uint32_t *x, size_t ncols, size_t nelem)
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
static int run_self_test(const CallerAPI *intf)
{
    uint32_t x_init[] = { // Input values
        0x03020100,  0x07060504,  0x0b0a0908,  0x0f0e0d0c,
        0x13121110,  0x17161514,  0x1b1a1918,  0x1f1e1d1c,
        0x00000001,  0x09000000,  0x4a000000,  0x00000000
    };    
    uint32_t out_final[] = { // Reference values from RFC 7359
       0xe4e7f110,  0x15593bd1,  0x1fdd0f50,  0xc47120a3,
       0xc7f4d1c7,  0x0368c033,  0x9aaa2204,  0x4e6cd4c3,
       0x466482d2,  0x09aa9f07,  0x05d7c214,  0xa2028bd9,
       0xd19c12b5,  0xb94e16de,  0xe883d0cb,  0x4e3c50a2
    };
    size_t mat32_map[] = { // Indexes for mapping 4x4 to 8x8 matrix
         0, 1, 2, 3,  0, 1, 2, 3,
         4, 5, 6, 7,  4, 5, 6, 7,
         8, 9,10,11,  8, 9,10,11,
        12,13,14,15, 12,13,14,15,
         0, 1, 2, 3,  0, 1, 2, 3,
         4, 5, 6, 7,  4, 5, 6, 7,
         8, 9,10,11,  8, 9,10,11,
        12,13,14,15, 12,13,14,15
    };

    ChaChaAVXState obj;

    ChaChaAVX_init(&obj, 20, x_init);
    for (size_t i = 0; i < 4; i++) {
        obj.x[i + 8]  = obj.x[i + 12] = x_init[i]; // Row 2
        obj.x[i + 16] = obj.x[i + 20] = x_init[i + 4]; // Row 3
        obj.x[i + 24] = obj.x[i + 28] = x_init[i + 8]; // Row 4
    }
    for (size_t i = 0; i < 32; i++) {
        obj.x[i + 32] = obj.x[i];
    }
    intf->printf("Input:\n"); print_matx(intf, obj.x, 8, 64);
    ChaChaAVX_block(&obj);
    intf->printf("Output (real):\n"); print_matx(intf, obj.out, 8, 64);
    intf->printf("Output (reference):\n"); print_matx(intf, out_final, 4, 16);
    for (int i = 0; i < 64; i++) {
        if (out_final[mat32_map[i]] != obj.out[i]) {
            intf->printf("TEST FAILED!\n");
            return 0;
        }        
    }
    intf->printf("Success.\n");
    return 1;
}

MAKE_UINT32_PRNG("ChaCha12", run_self_test)
