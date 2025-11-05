/**
 * @file blabla.c
 * @brief BlaBla counter-based pseudorandom number generator.
 * @details It was developed by JP Aumasson, one of the developers of the
 * BLAKE/BLAKE2/BLAKE3 hash functions and it is based on its cryptographic
 * compression function. The design is very similar to the ChaCha stream
 * cipher. But BlaBla cryptographic analysis has not been not published
 * and its real cryptographic quality is unknown.
 *
 * References:
 * 1. https://github.com/veorq/blabla/blob/master/BlaBla.swift
 * 2. RFC7693. The BLAKE2 Cryptographic Hash and Message Authentication Code
 *    (MAC)https://datatracker.ietf.org/doc/html/rfc7693.html
 * 3. Jean-Philippe Aumasson. Too Much Crypto // Cryptology ePrint Archive.
 *    2019. Paper 2019/1492. year = {2019}, https://eprint.iacr.org/2019/1492
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
#include <inttypes.h>

PRNG_CMODULE_PROLOG


enum {
    GEN_NROUNDS_REDUCED = 2,
    GEN_NROUNDS_MONTECARLO = 4,
    GEN_NROUNDS_FULL = 10
};


/**
 * @brief BlaBla counter-based PRNG state. Keeps two copies of this generator,
 * portable and SIMD (AVX2) implementations use the same state.
 */
typedef struct {
    uint64_t x[32];   ///< Working state
    uint64_t out[32]; ///< Output buffer
    size_t nrounds;   ///< Number of rounds
    size_t pos;       ///< Current position
} BlaBlaState;

/**
 * @brief Initializes BlaBla input block using the layout and constants from the
 * reference implementation. That constants differ from the ones used in BLAKE2b,
 * the upper halves of "Row 0" are taken from ChaCha, the origin of other bits
 * is unknown.
 */
static void blabla_init_block(uint64_t *x, const uint64_t *key, uint64_t ctr)
{
    // Row 0: IV
    x[0]  = 0x6170786593810fab; x[1]  = 0x3320646ec7398aee;
    x[2]  = 0x79622d3217318274; x[3]  = 0x6b206574babadada;
    // Row 1: key/seed    
    x[4]  = key[0]; x[5]  = key[1]; x[6]  = key[2]; x[7]  = key[3];
    // Row 1: IV
    x[8]  = 0x2ae36e593e46ad5f; x[9]  = 0xb68f143029225fc9;
    x[10] = 0x8da1e08468303aa6; x[11] = 0xa48a209acd50a4a7;
    // Row 2: IV and counter
    x[12] = 0x7fdc12f23f90778c; x[13] = ctr; x[14] = 0; x[15] = 0;
}

void BlaBlaState_init(BlaBlaState *obj, const uint64_t *key)
{
    blabla_init_block(obj->x, key, 1);
    blabla_init_block(obj->x + 16, key, 2);
    obj->nrounds = GEN_NROUNDS_FULL;
    obj->pos = 0;
}

static inline void BlaBlaState_inc_counter(BlaBlaState *obj)
{
    obj->x[13] += 2;
    obj->x[16 + 13] += 2;
}


////////////////////////////////////////////
///// Vectorized (AVX2) implementation /////
////////////////////////////////////////////

#ifdef __AVX2__
#include "smokerand/x86exts.h"
static inline __m256i mm256_roti_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi64(in, 64 - r), _mm256_srli_epi64(in, r));
}


static inline __m256i mm256_rot24_epi64_def(__m256i in)
{
    return _mm256_shuffle_epi8(in, _mm256_set_epi8(
        26, 25, 24, 31,  30, 29, 28, 27,    18, 17, 16, 23,  22, 21, 20, 19,
        10,  9,  8, 15,  14, 13, 12, 11,     2,  1,  0,  7,   6,  5,  4,  3));
}


static inline __m256i mm256_rot16_epi64_def(__m256i in)
{
    return _mm256_shuffle_epi8(in, _mm256_set_epi8(
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

static inline void blabla_round_avx2(__m256i *a, __m256i *b, __m256i *c, __m256i *d)
{
    // Vertical qround
    gfunc_avx2(a, b, c, d);
    // Diagonal qround; the original vector is [3 2 1 0]
    *b = _mm256_permute4x64_epi64(*b, 0x39); // [0 3 2 1] -> 3 (or <- 1)
    *c = _mm256_permute4x64_epi64(*c, 0x4E); // [1 0 3 2] -> 2 (or <- 2)
    *d = _mm256_permute4x64_epi64(*d, 0x93); // [2 1 0 3] -> 1 (or <- 3)
    gfunc_avx2(a, b, c, d);
    *b = _mm256_permute4x64_epi64(*b, 0x93);
    *c = _mm256_permute4x64_epi64(*c, 0x4E);
    *d = _mm256_permute4x64_epi64(*d, 0x39);
}
#endif


void EXPORT BlaBlaState_block_vector(BlaBlaState *obj)
{
#ifdef __AVX2__
    const __m256i *w256x = (__m256i *) (void *) (&obj->x[0]);
    __m256i *w256o = (__m256i *) (void *) (&obj->out[0]);

    __m256i in[8], x[8];
    for (size_t i = 0; i < 8; i++) {
        x[i] = _mm256_loadu_si256(&w256x[i]);
        in[i] = x[i];
    }

    for (size_t k = 0; k < obj->nrounds; k++) {
        blabla_round_avx2(&x[0], &x[1], &x[2], &x[3]);
        blabla_round_avx2(&x[4], &x[5], &x[6], &x[7]);
    }
    for (size_t i = 0; i < 8; i++) {
        x[i] = _mm256_add_epi64(x[i], in[i]);
        _mm256_storeu_si256(&w256o[i], x[i]);
    }
#else
    (void) obj;
#endif
}


static inline uint64_t get_bits_vector_raw(void *state)
{
    BlaBlaState *obj = state;
    uint64_t x = obj->out[obj->pos++];
    if (obj->pos == 32) {
        BlaBlaState_inc_counter(obj);
        BlaBlaState_block_vector(obj);
        obj->pos = 0;
    }
    return x;
}

MAKE_GET_BITS_WRAPPERS(vector);


//////////////////////////////////////////
///// Portable scalar implementation /////
//////////////////////////////////////////

static inline void gfunc(uint64_t *x, size_t ai, size_t bi, size_t ci, size_t di)
{
    x[ai] += x[bi]; x[di] = rotr64(x[di] ^ x[ai], 32);
    x[ci] += x[di]; x[bi] = rotr64(x[bi] ^ x[ci], 24);
    x[ai] += x[bi]; x[di] = rotr64(x[di] ^ x[ai], 16);
    x[ci] += x[di]; x[bi] = rotr64(x[bi] ^ x[ci], 63);

}

static inline void blabla_round_scalar(uint64_t *out)
{
    // Vertical permutations
    gfunc(out, 0, 4,  8, 12);
    gfunc(out, 1, 5,  9, 13);
    gfunc(out, 2, 6, 10, 14); 
    gfunc(out, 3, 7, 11, 15);
    // Diagonal permutations
    gfunc(out, 0, 5, 10, 15);
    gfunc(out, 1, 6, 11, 12);
    gfunc(out, 2, 7,  8, 13);
    gfunc(out, 3, 4,  9, 14);
}

EXPORT void BlaBlaState_block_scalar(BlaBlaState *obj)
{
    for (size_t i = 0; i < 32; i++) {
        obj->out[i] = obj->x[i];
    }
    for (size_t i = 0; i < obj->nrounds; i++) {
        blabla_round_scalar(obj->out);
        blabla_round_scalar(obj->out + 16);
    }
    for (size_t i = 0; i < 32; i++) {
        obj->out[i] += obj->x[i];
    }
}



static inline uint64_t get_bits_scalar_raw(void *state)
{
    BlaBlaState *obj = state;
    uint64_t x = obj->out[obj->pos++];
    if (obj->pos == 32) {
        BlaBlaState_inc_counter(obj);
        BlaBlaState_block_scalar(obj);
        obj->pos = 0;
    }
    return x;
}


MAKE_GET_BITS_WRAPPERS(scalar);

//////////////////////
///// Interfaces /////
//////////////////////


static void *create_generic(const CallerAPI *intf, int nrounds)
{
    uint64_t key[4];
    BlaBlaState *obj = intf->malloc(sizeof(BlaBlaState));
    for (size_t i = 0; i < 4; i++) {
        key[i] = intf->get_seed64();
    }
    BlaBlaState_init(obj, key);
    obj->nrounds = nrounds;
    BlaBlaState_block_scalar(obj);
    return obj;
}

static void *create(const CallerAPI *intf)
{
    return create_generic(intf, GEN_NROUNDS_FULL);
}


static void *create_reduced(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_generic(intf, GEN_NROUNDS_REDUCED);
}

static void *create_montecarlo(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_generic(intf, GEN_NROUNDS_MONTECARLO);
}


static int compare_data(const CallerAPI *intf, BlaBlaState *obj,
    uint64_t (*get_bits)(void *state), const uint64_t *ref_data, size_t len)
{
    int is_ok = 1;
    for (size_t i = 0; i < len; i++) {
        uint64_t u = get_bits(obj);
        intf->printf("%3d %16" PRIx64 " %16" PRIx64, (int) i, u, ref_data[i]);
        if (ref_data[i] != u) {
            is_ok = 0;
            intf->printf(" <--\n");
        } else {
            intf->printf("\n");
        }
    }
    return is_ok;
}


/**
 * @brief An internal self-test for BlaBla PRNG (the original 10-round version)
 * @details The constants were dumped from the reference implementation written
 * in Swift programming language.
 *
 * The first bytes from the keystream of the reference implementation:
 *
 * - Decimal: 173, 80, 254, 123, 103, 188, 241, 234
 * - Hexadecimal: ad 50 fe 7b 67 bc f1 ea
 * - 64-bit word: 0xeaf1bc677bfe50ad
 */
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

    intf->printf("----- Checking the scalar (C99) version -----\n");
    BlaBlaState_init(obj, key);
    BlaBlaState_block_scalar(obj);
    int is_ok = compare_data(intf, obj, get_bits_scalar, data, 64);
    intf->printf("----- Checking the vectorized (AVX2) version -----\n");
    BlaBlaState_init(obj, key);
    BlaBlaState_block_vector(obj);
    is_ok = is_ok & compare_data(intf, obj, get_bits_vector, data, 64);
    intf->free(obj);
    return is_ok;
}


static const char description[] =
"BlaBla counter-based PRNG based on BLAKE2b compression function, suggested\n"
"by J.P. Aumasson. Essentially a modification of ChaCha for 64-bit words.\n"
"The next param values are supported:\n"
"  c99             - portable BlaBla version (default, slower): 10 rounds\n"
"  avx2            - AVX2 BlaBla version (fastest): 10 rounds\n"
"  c99-montecarlo  - c99 with reduced number of rounds: 4 rounds\n"
"  avx2-montecarlo - avx2 with reduced number of rounds: 4 rounds\n"
"  c99-reduced     - c99 with reduced number of rounds: 2 rounds\n"
"  avx2-reduced    - avx2 with reduced number of rounds: 2 rounds\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 64;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "c99") || !intf->strcmp(param, "")) {
        gi->name = "BlaBla:c99";
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "avx2")) {
        gi->name = "BlaBla:avx2";
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
#ifndef __AVX2__
        intf->printf("Not implemented\n");
        return 0;
#endif
    } else if (!intf->strcmp(param, "c99-montecarlo")) {
        gi->name = "BlaBla:c99:montecarlo";
        gi->create = create_montecarlo;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "avx2-montecarlo")) {
        gi->name = "BlaBla:avx2:montecarlo";
        gi->create = create_montecarlo;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
#ifndef __AVX2__
        intf->printf("Not implemented\n");
        return 0;
#endif
    } else if (!intf->strcmp(param, "c99-reduced")) {
        gi->name = "BlaBla:c99:reduced";
        gi->create = create_reduced;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "avx2-reduced")) {
        gi->name = "BlaBla:avx2:reduced";
        gi->create = create_reduced;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
#ifndef __AVX2__
        intf->printf("Not implemented\n");
        return 0;
#endif
    } else {
        gi->name = "BlaBla:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
        return 0;
    }
    return 1;
}
