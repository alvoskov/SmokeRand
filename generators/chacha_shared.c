/**
 * @file chacha_shared.c
 * @brief ChaCha12 pseudorandom number generator. 
 * @details ChaCha12 PRNG is a modification of cryptographically secure
 * ChaCha20 with reduced number of rounds. Can be considered as CSPRNG
 * itself. ChaCha12 should pass all statistical tests from the TestU01
 * library.
 *
 * References:
 * 1. RFC 7539. ChaCha20 and Poly1305 for IETF Protocols
 *    https://datatracker.ietf.org/doc/html/rfc7539
 *
 * NOTE: This PRNG also must pass the test implemented in chacha_shared.c.
 * This test switches it to the ChaCha20 mode and compares output with
 * RFC 7539 reference values.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

//#ifdef  __AVX__
//#define CHACHA_VECTOR_INTR
//#endif

static const int gen_nrounds = 12;

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
    uint32_t x[16]; ///< Working state
    uint32_t out[16]; ///< Output state
    size_t ncycles; ///< Number of rounds / 2
    size_t pos;
} ChaChaState;


//////////////////////////////
///// CPU-dependent code /////
//////////////////////////////

#ifdef CHACHA_VECTOR_INTR
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

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
static inline void mm_qround_vert(__m128i *a, __m128i *b, __m128i *c, __m128i *d)
{
    *a = _mm_add_epi32(*a, *b); *d = _mm_xor_si128(*d, *a); *d = mm_rot16_epi32_def(*d);
    *c = _mm_add_epi32(*c, *d); *b = _mm_xor_si128(*b, *c); *b = mm_roti_epi32_def(*b, 12);
    *a = _mm_add_epi32(*a, *b); *d = _mm_xor_si128(*d, *a); *d = mm_rot8_epi32_def(*d);
    *c = _mm_add_epi32(*c, *d); *b = _mm_xor_si128(*b, *c); *b = mm_roti_epi32_def(*b, 7);
}
#else

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
#endif

/**
 * @brief Increase the value of 128-bit PRNG counter.
 */
static inline void ChaCha_inc_counter(ChaChaState *obj)
{
    // Variant with 32-bit counter for testing purposes
    // (will fail gap test!)
    //obj->x[12]++;
    // 128-bit counter
    uint64_t *cnt = (uint64_t *) &obj->x[12];
    if (++cnt[0] == 0) ++cnt[1];
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
void ChaCha_block(ChaChaState *obj)
{
#ifdef CHACHA_VECTOR_INTR
    __m128i a = _mm_loadu_si128((__m128i *) obj->x);
    __m128i b = _mm_loadu_si128((__m128i *) (obj->x + 4));
    __m128i c = _mm_loadu_si128((__m128i *) (obj->x + 8));
    __m128i d = _mm_loadu_si128((__m128i *) (obj->x + 12));
    __m128i ax = a, bx = b, cx = c, dx = d;
    for (size_t k = 0; k < obj->ncycles; k++) {
        /* Vertical qround */
        mm_qround_vert(&a, &b, &c, &d);
        /* Diagonal qround; the original vector is [3 2 1 0] */
        b = _mm_shuffle_epi32(b, 0x39); // [0 3 2 1] -> 3 (or <- 1)
        c = _mm_shuffle_epi32(c, 0x4E); // [1 0 3 2] -> 2 (or <- 2)
        d = _mm_shuffle_epi32(d, 0x93); // [2 1 0 3] -> 1 (or <- 3)
        mm_qround_vert(&a, &b, &c, &d);
        b = _mm_shuffle_epi32(b, 0x93);
        c = _mm_shuffle_epi32(c, 0x4E);
        d = _mm_shuffle_epi32(d, 0x39);
    }
    a = _mm_add_epi32(a, ax);
    b = _mm_add_epi32(b, bx);
    c = _mm_add_epi32(c, cx);
    d = _mm_add_epi32(d, dx);

    _mm_storeu_si128((__m128i *) obj->out, a);
    _mm_storeu_si128((__m128i *) (obj->out + 4), b);
    _mm_storeu_si128((__m128i *) (obj->out + 8), c);
    _mm_storeu_si128((__m128i *) (obj->out + 12), d);
#else
    for (size_t k = 0; k < 16; k++) {
        obj->out[k] = obj->x[k];
    }

    for (size_t k = 0; k < obj->ncycles; k++) {
        /* Vertical qrounds */
        qround(obj->out, 0, 4, 8,12);
        qround(obj->out, 1, 5, 9,13);
        qround(obj->out, 2, 6,10,14);
        qround(obj->out, 3, 7,11,15);
        /* Diagonal qrounds */
        qround(obj->out, 0, 5,10,15);
        qround(obj->out, 1, 6,11,12);
        qround(obj->out, 2, 7, 8,13);
        qround(obj->out, 3, 4, 9,14);
    }
    for (size_t i = 0; i < 16; i++) {
        obj->out[i] += obj->x[i];
    }
#endif
}

/**
 * @brief Initialize the state of ChaCha CSPRNG.
 * The function is exported for debugging purposes.
 * @param obj     The state to be initialized.
 * @param nrounds Number of rounds (8, 12, 20).
 * @param seed    Pointer to array of 8 uint32_t values (seeds).
 */
void ChaCha_init(ChaChaState *obj, size_t nrounds, const uint32_t *seed)
{
    /* Constants: the upper row of the matrix */
    obj->x[0] = 0x61707865; obj->x[1] = 0x3320646e;
    obj->x[2] = 0x79622d32; obj->x[3] = 0x6b206574;
    /* Rows 1-2: seed (key) */
    for (size_t i = 0; i < 8; i++) {
        obj->x[i + 4] = seed[i];
    }
    /* Row 3: counter and nonce */
    for (size_t i = 12; i <= 15; i++) {
        obj->x[i] = 0;
    }
    ChaCha_inc_counter(obj);
    /* Number of rounds => Number of cycles */
    obj->ncycles = nrounds / 2;
    /* Output state */
    for (size_t i = 0; i < 16; i++) {
        obj->out[i] = 0;
    }
    /* Output counter */
    obj->pos = 16;
}

static inline uint64_t get_bits_raw(void *state)
{
    ChaChaState *obj = state;
    if (obj->pos >= 16) {
        ChaCha_inc_counter(obj);
        ChaCha_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    ChaChaState *obj = intf->malloc(sizeof(ChaChaState));
    uint32_t seeds[8];
    for (int i = 0; i < 4; i++) {
        uint64_t s = intf->get_seed64();
        seeds[2*i] = s & 0xFFFFFFF;
        seeds[2*i + 1] = s >> 32;
    }
    ChaCha_init(obj, gen_nrounds, seeds);
    return (void *) obj;
}

/**
 * @brief Print the 4x4 matrix of uint32_t from the ChaCha PRNG state.
 * @param x Pointer to the matrix (C-style)
 */
static void print_mat16(const CallerAPI *intf, uint32_t *x)
{
    for (int i = 0; i < 16; i++) {
        intf->printf("%10.8X ", x[i]);
        if ((i + 1) % 4 == 0)
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
    uint32_t out_final[] = { // Refernce values from RFC 7359
       0xe4e7f110,  0x15593bd1,  0x1fdd0f50,  0xc47120a3,
       0xc7f4d1c7,  0x0368c033,  0x9aaa2204,  0x4e6cd4c3,
       0x466482d2,  0x09aa9f07,  0x05d7c214,  0xa2028bd9,
       0xd19c12b5,  0xb94e16de,  0xe883d0cb,  0x4e3c50a2
    };
    ChaChaState obj;
    ChaCha_init(&obj, 20, x_init); // 20 rounds
    for (int i = 0; i < 12; i++) {
        obj.x[i + 4] = x_init[i];
    }
    intf->printf("Input:\n"); print_mat16(intf, obj.x);
    ChaCha_block(&obj);
    intf->printf("Output (real):\n"); print_mat16(intf, obj.out);
    intf->printf("Output (reference):\n"); print_mat16(intf, out_final);
    for (int i = 0; i < 16; i++) {
        if (out_final[i] != obj.out[i]) {
            intf->printf("TEST FAILED!\n");
            return 0;
        }        
    }
    intf->printf("Success.\n");
    return 1;
}

MAKE_UINT32_PRNG("ChaCha12", run_self_test)
