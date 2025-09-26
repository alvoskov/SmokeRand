/**
 * @file blabla.c
 * @brief BlaBla counter-based pseudorandom number generator.
 * @details It was developed by JP Aumasson, one of the developers of the
 * BLAKE/BLAKE2/BLAKE3 hash functions and it is based on its cryptographic
 * compression function. The design is very similar to ChaCha stream cipher
 * but it is unknown if BlaBla is a cryptographically strong PRNG (but it
 * probably may be).
 *
 * References:
 * 1. https://github.com/veorq/blabla/blob/master/BlaBla.swift
 *
 * @copyright The BlaBla algorithm was suggested by JP Aumasson.
 * 
 * Adaptation for C and SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */


#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x[16];   ///< Working state
    uint64_t out[16]; ///< Output buffer
    size_t nrounds;   ///< Number of rounds
    size_t pos;       ///< Current position
} BlaBlaState;



#ifdef __AVX2__
#include "smokerand/x86exts.h"
static inline __m256i mm256_roti_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi64(in, 64 - r), _mm256_srli_epi64(in, r));
}


static inline __m256i mm256_rot24_epi64_def(__m256i in)
{
    return _mm256_shuffle_epi8(in,                     
        _mm256_set_epi8(
            26, 25, 24, 31,  30, 29, 28, 27,    18, 17, 16, 23,  22, 21, 20, 19,
            10,  9,  8, 15,  14, 13, 12, 11,     2,  1,  0,  7,   6,  5,  4,  3));
}


static inline __m256i mm256_rot16_epi64_def(__m256i in)
{

    return _mm256_shuffle_epi8(in,                     
        _mm256_set_epi8(
            25, 24, 31, 30,  29, 28, 27, 26,    17, 16, 23, 22,  21, 20, 19, 18,
             9,  8, 15, 14,  13, 12, 11, 10,     1,  0, 7,  6,    5,  4,  3,  2));
}


static inline void gfunc_avx2(__m256i *a, __m256i *b, __m256i *c, __m256i *d)
{
    *a = _mm256_add_epi64(*a, *b);
    *d = _mm256_xor_si256(*d, *a);
    *d = _mm256_shuffle_epi32(*d, 0xB1);

    *c = _mm256_add_epi64(*c, *d);
    *b = _mm256_xor_si256(*b, *c);
    *b = mm256_rot24_epi64_def(*b);

    *a = _mm256_add_epi64(*a, *b);
    *d = _mm256_xor_si256(*d, *a);
    *d = mm256_rot16_epi64_def(*d);

    *c = _mm256_add_epi64(*c, *d);
    *b = _mm256_xor_si256(*b, *c);
    *b = mm256_roti_epi64_def(*b, 63);
}
#endif



void EXPORT BlaBlaVec_block(BlaBlaState *obj)
{
#ifdef __AVX2__
    const __m256i *w256x = (__m256i *) (&obj->x[0]);
    __m256i *w256o = (__m256i *) (&obj->out[0]);
    __m256i a = _mm256_loadu_si256(&w256x[0]); // words 0..3
    __m256i b = _mm256_loadu_si256(&w256x[1]); // words 4..7
    __m256i c = _mm256_loadu_si256(&w256x[2]); // words 8..11
    __m256i d = _mm256_loadu_si256(&w256x[3]); // words 12..15

    __m256i ax = a, bx = b, cx = c, dx = d;
    for (size_t k = 0; k < obj->nrounds; k++) {
        // --- Generator 0 of 1
        // Vertical qround
        gfunc_avx2(&a, &b, &c, &d);
        // Diagonal qround; the original vector is [3 2 1 0]
        b = _mm256_permute4x64_epi64(b, 0x39); // [0 3 2 1] -> 3 (or <- 1)
        c = _mm256_permute4x64_epi64(c, 0x4E); // [1 0 3 2] -> 2 (or <- 2)
        d = _mm256_permute4x64_epi64(d, 0x93); // [2 1 0 3] -> 1 (or <- 3)
        gfunc_avx2(&a, &b, &c, &d);
        b = _mm256_permute4x64_epi64(b, 0x93);
        c = _mm256_permute4x64_epi64(c, 0x4E);
        d = _mm256_permute4x64_epi64(d, 0x39);
        // --- Generator 1 of 1
        // TODO: ADD IT!
    }
    a = _mm256_add_epi64(a, ax);
    b = _mm256_add_epi64(b, bx);
    c = _mm256_add_epi64(c, cx);
    d = _mm256_add_epi64(d, dx);

    _mm256_storeu_si256(&w256o[0], a);
    _mm256_storeu_si256(&w256o[1], b);
    _mm256_storeu_si256(&w256o[2], c);
    _mm256_storeu_si256(&w256o[3], d);
#else
    (void) obj;
#endif
}




static inline void gfunc(uint64_t *x, size_t ai, size_t bi, size_t ci, size_t di)
{
    x[ai] += x[bi]; x[di] = rotr64(x[di] ^ x[ai], 32);
    x[ci] += x[di]; x[bi] = rotr64(x[bi] ^ x[ci], 24);
    x[ai] += x[bi]; x[di] = rotr64(x[di] ^ x[ai], 16);
    x[ci] += x[di]; x[bi] = rotr64(x[bi] ^ x[ci], 63);

}

EXPORT void BlaBlaState_block(BlaBlaState *obj)
{
    return BlaBlaVec_block(obj);
#if 0
    for (size_t i = 0; i < 16; i++) {
        obj->out[i] = obj->x[i];
    }
    for (size_t i = 0; i < obj->nrounds; i++) {
        // Vertical permutations
        gfunc(obj->out, 0, 4,  8, 12);
        gfunc(obj->out, 1, 5,  9, 13);
        gfunc(obj->out, 2, 6, 10, 14); 
        gfunc(obj->out, 3, 7, 11, 15);
        // Diagonal permutations
        gfunc(obj->out, 0, 5, 10, 15);
        gfunc(obj->out, 1, 6, 11, 12);
        gfunc(obj->out, 2, 7,  8, 13);
        gfunc(obj->out, 3, 4,  9, 14);

    }
    for (size_t i = 0; i < 16; i++) {
        obj->out[i] += obj->x[i];
    }
#endif
}

void BlaBlaState_init(BlaBlaState *obj, const uint64_t *key)
{
    // Row 0: IV
    obj->x[0]  = 0x6170786593810fab;
    obj->x[1]  = 0x3320646ec7398aee;
    obj->x[2]  = 0x79622d3217318274;
    obj->x[3]  = 0x6b206574babadada;
    // Row 1: key/seed    
    obj->x[4]  = key[0];
    obj->x[5]  = key[1];
    obj->x[6]  = key[2];
    obj->x[7]  = key[3];
    // Row 1: IV
    obj->x[8]  = 0x2ae36e593e46ad5f;
    obj->x[9]  = 0xb68f143029225fc9;
    obj->x[10] = 0x8da1e08468303aa6;
    obj->x[11] = 0xa48a209acd50a4a7;
    // Row 2: IV and counter
    obj->x[12] = 0x7fdc12f23f90778c;
    obj->x[13] = 1;
    obj->x[14] = 0;
    obj->x[15] = 0;
    // Other settings
    obj->nrounds = 10;
    obj->pos = 0;
    // Initialization
    BlaBlaState_block(obj);
}



static inline uint64_t get_bits_raw(void *state)
{
    BlaBlaState *obj = state;
    uint64_t x = obj->out[obj->pos++];
    if (obj->pos == 16) {
        obj->x[13]++;
        BlaBlaState_block(obj);
        obj->pos = 0;
    }
    return x;
}


static void *create(const CallerAPI *intf)
{
    uint64_t key[4];
    BlaBlaState *obj = intf->malloc(sizeof(BlaBlaState));
    for (size_t i = 0; i < 4; i++) {
        key[i] = intf->get_seed64();
    }
    BlaBlaState_init(obj, key);
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    BlaBlaState *obj = intf->malloc(sizeof(BlaBlaState));
    uint64_t key[4] = {0x0706050403020100, 0x0f0e0d0c0b0a0908,
        0x1716151413121110, 0x1f1e1d1c1b1a1918};

    static const uint64_t data[64] = {
        0xeaf1bc677bfe50ad, 0x6303565fc99a8210, 0x14b888eeeedaaf48, 0x0cd821373adf2a85, // Block 1
        0xe3770bc137b970a6, 0x7650c54a957c3a92, 0x615b893daed9da00, 0x2559c8bd35d31028,
        0x1d1b5802b22e658d, 0x4c637a651c694e0d, 0xb3bb6da9f05756fc, 0x19ded05a3310f8c0,
        0xaf1c0fb092d13d00, 0xdafb7d4327eb7d2b, 0xaaddfb9cdb034287, 0x1e74cae786541a89,

        0x17e911c3a6920c7e, 0x75a5da3f93ec7e32, 0xf68e45bfafdbda25, 0xd1e73d8ee411b262, // Block 2
        0xaa1953d91fc33243, 0xbb5e0d667752dee9, 0xa9be74d9e90ea93e, 0xe976e7ba9e262cb2,
        0x62a9ea697b6cc0ec, 0xa6723b6ebe578bd0, 0x092f18c05eae4472, 0xca9418e79954db95,
        0x2def4e9c25eed2cc, 0x2ff09cb62690502a, 0xf1f2ca23720863aa, 0xe02b4830ef1566c6,

        0xf1e04f5284d74f1d, 0x94469451f1be14a1, 0xe0e229051e2f6b58, 0xf2dd609f32605f51, // Block 3
        0xc460990c6d16d611, 0xb07dee8b5a6d6606, 0xb5fed4f149b1de39, 0x6f748d4c6cb5fb1b,
        0x24a3b4bacaecd91d, 0x0dea24a9b62e465d, 0xa06324cbbcec555d, 0x4569ab3a647280e3,
        0xa4fa01d0c8ccfe43, 0x34b21d77d3835407, 0xed4b8ff99705abd8, 0x6e512a2429630f91,

        0x0d19448d461c814e, 0x524dd1fc63a701ea, 0x9b613ec027e97810, 0x5d026ec18bf1c791, // Block 4
        0x30e18f49e91a8445, 0x92dd040c4eeb6252, 0xb570d3cf48c70614, 0x8d87c8f88aab350d,
        0xca867878b8a11658, 0xd42934043914dbe6, 0x08f3989881cfab23, 0xc28ca8ef3571e185,
        0x704b71035bfcc609, 0xcc8b25946643dc2c, 0xc8b05535a4c0871e, 0x06e8049d2270f063
    };

    BlaBlaState_init(obj, key);
    int is_ok = 1;
    BlaBlaState_block(obj);
    for (size_t i = 0; i < 64; i++) {
        uint64_t u = get_bits_raw(obj);
        intf->printf("%3d %16llX %16llX\n", (int) i, u, data[i]);
        if (data[i] != u) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}



MAKE_UINT64_PRNG("BlaBla", run_self_test)
