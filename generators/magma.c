/**
 * @file magma.c
 * @brief CSPRNG based on "Magma" from GOST R 34.12-2015 (version for AVX2)
 * @details "Magma" from GOST R 34.12-2015 is a 64-bit block cipher developed
 * in USSR. It is a Feistel cipher that uses 8 4x4 S-boxes.
 *
 * This file contains two implementations of MAGMA:
 *
 * 1. The cross-platform one. However, it has very low performance around
 *    25 cpb (10-20 times slower than ChaCha12) and uses around 2KiB of RAM
 *    for unrolled permutation tables.
 * 2. Vectorized version that is designed for x86-64 processors with AVX2
 *    instructions set. It is and 3-4 times faster than the cross-platform
 *    non-vectorized version. However it is still slow, around 7 cpb.
 *
 * Reduced round version testing of AVX CTR version:
 * - 8 rounds: passes `express` but not `default`
 * - 10 rounds: almost passes `default` (systematically low p-values at
 *   `hamming_ot`)
 * - 11 rounds: passes `default`
 * - 12 rounds: passes `full`
*
 * References:
 *
 * 1. RFC8891. GOST R 34.12-2015: Block Cipher "Magma"
 *    https://datatracker.ietf.org/doc/html/rfc8891
 * 2. Ludmila Babenko, Evgeniya Ishchukova, and Ekaterina Maro. 2012. Research
 *    about strength of GOST 28147-89 encryption algorithm. In Proceedings of
 *    the Fifth International Conference on Security of Information and
 *    Networks (SIN '12). Association for Computing Machinery, New York, NY,
 *    USA, 138-142. https://doi.org/10.1145/2388576.2388595
 * 3. GOST R 34.12-2015 (in Russian)
 *    https://tc26.ru/standard/gost/GOST_R_3412-2015.pdf
 * 4. Ryad Benadjila, Jian Guo, Victor Lomne, Thomas Peyrin. Implementing
 *    Lightweight Block Ciphers on x86 Architectures // Cryptology ePrint
 *    Archive, Paper 2013/445. 2013. https://eprint.iacr.org/2013/445.
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#ifdef __AVX2__
#define MAGMA_VEC_ENABLED
#include "smokerand/x86exts.h"
#endif

PRNG_CMODULE_PROLOG


typedef union {
    uint32_t w32[8];
    uint64_t w64[4];
    uint8_t w8[32];
} Vector256;


/**
 * @brief MAGMA-AVX-GOSTR34.12-2015 CSPRNG operation mode.
 */
typedef enum {
    MAGMA_CTR, ///< Counter mode (CTR)
    MAGMA_CBC  ///< Cipher block chaining (CBC)
} MagmaMode;


/**
 * @brief MAGMA-AVX-GOSTR34.12-2015 CSPRNG state: scalar version.
 */
typedef struct {
    uint32_t sbox8[4][256];
    Vector256 key;
    uint64_t ctr;
} MagmaState;


/**
 * @brief MAGMA-AVX-GOSTR34.12-2015 CSPRNG state: vectorized (AVX2) version.
 */
typedef struct {
    Vector256 key; ///< 256-bit encryption key (PRNG seed)
    Vector256 ctr_a0; ///< Lower halves of 8 counters
    Vector256 ctr_a1; ///< Upper halves of 8 counters
    Vector256 out_a0; ///< Buffer for output data (encrypted counters)
    Vector256 out_a1; ///< Buffer for output data (encrypted counters)
    MagmaMode mode; ///< Current cipher mode
    int pos; ///< Current position in the output buffer
} MagmaVecState;


/////////////////////////////////////////
///// Scalar version implementation /////
/////////////////////////////////////////


static const unsigned char sbox4[8][16] = {
    {12,  4,  6,  2, 10,  5, 11,  9, 14,  8, 13,  7,  0,  3, 15,  1}, // 0
    { 6,  8,  2,  3,  9, 10,  5, 12,  1, 14,  4,  7,  1, 13,  0, 15},
    {11,  3,  5,  8,  2, 15, 10, 13, 14,  1,  7,  4, 12,  9,  6,  0},
    {12,  8,  2,  1, 13,  4, 15,  6,  7,  0, 10,  5,  3, 14,  9, 11},
    { 7, 15,  5, 10,  8,  1,  6, 13,  0,  9,  3, 14, 11,  4,  2, 12},
    { 5, 13, 15,  6,  9,  2, 12, 10, 11,  7,  8,  1,  4,  3, 14,  0},
    { 8, 14,  2,  5,  6,  9,  1, 12, 15,  4, 11,  0, 13, 10,  3,  7},
    { 1,  7, 14, 13,  0,  5,  8,  3,  4, 15, 10,  6,  9, 12, 11,  2} // 7
};

void MagmaState_init(MagmaState *obj, const uint32_t *key)
{
    for (int i = 0; i < 4; i++) {
        for (int j1 = 0; j1 < 16; j1++) {
            for (int j2 = 0; j2 < 16; j2++) {
                int index = (j1 << 4) | j2;
                uint32_t s = (sbox4[2*i + 1][j1] << 4) | sbox4[2*i][j2];
                s <<= (8 * i);
                s = (s << 11) | (s >> 21);
                obj->sbox8[i][index] = s;
            }
        }
    }
    for (int i = 0; i < 8; i++) {
        obj->key.w32[i] = key[i];
    }
    obj->ctr = 0;
}

static inline uint32_t gfunc(const MagmaState *obj, uint32_t k, uint32_t x)
{
    uint32_t y;
    x += k;
    y  = obj->sbox8[0][x & 0xFF]; //x >>= 8;
    y |= obj->sbox8[1][(x >> 8) & 0xFF]; //x >>= 8;
    y |= obj->sbox8[2][(x >> 16) & 0xFF]; //x >>= 8;
    y |= obj->sbox8[3][(x >> 24)];
    return y;
}

#define MAGMA_ROUND(ind) { t = a1 ^ gfunc(obj, a0, obj->key.w32[ind]); a1 = a0; a0 = t; }

EXPORT uint64_t MagmaState_encrypt(const MagmaState *obj, uint64_t a)
{
    uint32_t a1 = a >> 32, a0 = (uint32_t) a;
    uint32_t t;
    for (int i = 0; i < 3; i++) {
        MAGMA_ROUND(0);  MAGMA_ROUND(1);  MAGMA_ROUND(2);  MAGMA_ROUND(3);
        MAGMA_ROUND(4);  MAGMA_ROUND(5);  MAGMA_ROUND(6);  MAGMA_ROUND(7);
    }
    MAGMA_ROUND(7);  MAGMA_ROUND(6);  MAGMA_ROUND(5);  MAGMA_ROUND(4);
    MAGMA_ROUND(3);  MAGMA_ROUND(2);  MAGMA_ROUND(1);  MAGMA_ROUND(0);
    return ( (uint64_t) a0 << 32) | a1;
}

static inline uint64_t get_bits_scalar_raw(void *state)
{
    MagmaState *obj = state;
    return MagmaState_encrypt(obj, obj->ctr++);
}

MAKE_GET_BITS_WRAPPERS(scalar)

static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    MagmaState *obj = intf->malloc(sizeof(MagmaState));
    uint32_t key[8];
    for (int i = 0; i < 4; i++) {
        uint64_t seed = intf->get_seed64();
        key[2*i] = seed >> 32;
        key[2*i + 1] = (uint32_t) seed;
    }
    MagmaState_init(obj, key);
    (void) gi;
    return obj;
}


static int run_self_test_scalar(const CallerAPI *intf)
{
    // key is taken from the original GOST.
    static const uint32_t key[] = {
        0xffeeddcc, 0xbbaa9988, 0x77665544, 0x33221100,
        0xf0f1f2f3, 0xf4f5f6f7, 0xf8f9fafb, 0xfcfdfeff
    };
    // Only the first ctr-u_ref pair is obtained from the original GOST.
    // Other pairs were generated by means of this (non-vectorized) MAGMA
    // implementation.
    static const uint64_t ctr_in[8] = {
        0xfedcba9876543210, 0x243F69A25B093B12, 0x24C5B22658595D69, 0x0000000000000000,
        0x0123456789ABCDEF, 0xB7E151628AED2A6B, 0xDEADBEEFDEADBEEF, 0xFFFFFFFFFFFFFFFF
    };
    static const uint64_t u_ref[8] = {
        0x4ee901e5c2d8ca3d, 0x55DAEE31ED87E6F7, 0xB644E51E09B20B3E, 0x1BB3E0C407A59322,
        0xA6CB0CB94195EA34, 0x13F900FFCBEEB4FE, 0x9E57E39D28EAC91A, 0x503C956F1519A1A3
    };
    int is_ok = 1;
    MagmaState *obj = intf->malloc(sizeof(MagmaState));
    MagmaState_init(obj, key);
    intf->printf("----- Scalar version internal self-test -----\n");
    for (int i = 0; i < 8; i++) {
        obj->ctr = ctr_in[i];
        uint64_t u = get_bits_scalar_raw(obj);
        intf->printf("Out = 0x%llX; ref = 0x%llX\n", u, u_ref[i]);
        if (u != u_ref[i]) is_ok = 0;
    }
    intf->free(obj);
    return is_ok;
};

/////////////////////////////////////////////
///// Vectorized version implementation /////
/////////////////////////////////////////////



void MagmaVecState_init(MagmaVecState *obj, const uint32_t *key)
{
    for (int i = 0; i < 8; i++) {
        obj->key.w32[i] = key[i];
        obj->ctr_a0.w32[i] = i;
        obj->ctr_a1.w32[i] = 0;
        obj->out_a0.w32[i] = 0; // Needed for CBC mode
        obj->out_a1.w32[i] = 0;
    }
    obj->mode = MAGMA_CTR;
    obj->pos = 8;
}

static void *create_vector(const CallerAPI *intf, MagmaMode mode)
{
#ifdef MAGMA_VEC_ENABLED
    MagmaVecState *obj = intf->malloc(sizeof(MagmaVecState));
    uint32_t key[8];
    for (int i = 0; i < 4; i++) {
        uint64_t seed = intf->get_seed64();
        key[2*i] = seed >> 32;
        key[2*i + 1] = (uint32_t) seed;
    }
    MagmaVecState_init(obj, key);
    obj->mode = mode;
    return obj;
#else
    (void) intf; (void) mode;
    return NULL;
#endif
}

static inline void Vector256_print(const Vector256 *obj, const CallerAPI *intf)
{
    intf->printf("  w32: ");
    for (int i = 0; i < 8; i++) {
        intf->printf("0x%8.8X ", obj->w32[i]);
    }
    intf->printf("\n  w8:  ");
    for (int i = 31; i >= 0; i--) {
        intf->printf("%2.2X ", (unsigned int) obj->w8[i]);
        if (i % 4 == 0)
            intf->printf(" ");
    }
    intf->printf("\n");
}


static void *create_vector_ctr(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_vector(intf, MAGMA_CTR);
}

static void *create_vector_cbc(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_vector(intf, MAGMA_CBC);
}

#ifdef MAGMA_VEC_ENABLED
static inline __m256i Vector256_to_m256i(const Vector256 *obj)
{
    return _mm256_loadu_si256((__m256i *) (void *) obj->w32);
}

static inline void Vector256_from_m256i(Vector256 *obj, __m256i x)
{
    _mm256_storeu_si256((__m256i *) (void *) obj->w32, x);
}

#define APPLY_SBOX_LO_TOOUT(mask4, mask32, ind) \
    out = _mm256_and_si256(x, _mm256_set1_epi32(mask4)); \
    out = _mm256_or_si256(out, _mm256_set1_epi32(mask32)); \
    out = _mm256_shuffle_epi8(sbox[ind], out);


#define APPLY_SBOX_LO(mask4, mask32, ind) { \
    __m256i ni = _mm256_and_si256(x, _mm256_set1_epi32(mask4)); \
    ni = _mm256_or_si256(ni, _mm256_set1_epi32(mask32)); \
    ni = _mm256_shuffle_epi8(sbox[ind], ni); \
    out = _mm256_or_si256(out, ni); \
}


#define APPLY_SBOX_HI(mask4, mask32, ind) { \
    __m256i ni = _mm256_and_si256(x, _mm256_set1_epi32(mask4)); \
    ni = _mm256_srli_epi32(ni, 4); \
    ni = _mm256_or_si256(ni, _mm256_set1_epi32(mask32)); \
    ni = _mm256_shuffle_epi8(sbox[ind], ni); \
    ni = _mm256_slli_epi32(ni, 4); \
    out = _mm256_or_si256(out, ni); \
}


static inline __m256i mm256_roti_epi32_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi32(in, r), _mm256_srli_epi32(in, 32 - r));
}


/**
 * @brief Rearranges (collects) 16-bit words in the 256-bit register:
 *
 * Inside 128-bit half:
 *
 *    ||3-2|1-0||3-2|1-0||3-2|1-0||3-2|1-0|| =>
 * => ||3-2|3-2||3-2|3-2||1-0|1-0||1-0|1-0||
 *
 * The whole 256-bit register (2nd state):
 *
 *     ||4 x (3-2) | 4 x (1-0) || 4 x (3-2) | 4 x (1-0) || =>
 * =>  ||4 x (3-2) | 4 x (3-2) || 4 x (1-0) | 4 x (1-0) || =>
 */
static inline __m256i collect_w16(__m256i x)
{
    __m256i w16grp = _mm256_shuffle_epi8(x,
        _mm256_set_epi8(
        15,14, 11,10, 7,6, 3,2,  13,12, 9,8, 5,4, 1,0,
        15,14, 11,10, 7,6, 3,2,  13,12, 9,8, 5,4, 1,0));
    return _mm256_permute4x64_epi64(w16grp, 0xD8); // 0b11_01_10_00
}

static inline __m256i uncollect_w16(__m256i x)
{
    __m256i w16grp = _mm256_permute4x64_epi64(x, 0xD8); // 0b11_01_10_00
    return _mm256_shuffle_epi8(w16grp,
        _mm256_set_epi8(
            15,14, 7,6, 13,12, 5,4,   11,10, 3,2, 9,8, 1,0,
            15,14, 7,6, 13,12, 5,4,   11,10, 3,2, 9,8, 1,0));

}

/**
 * @brief Nonlinear transformation of the cipher. Contains S-boxes that
 * are applied to the 8 32-bit words using AVX2 instructions.
 */
static inline __m256i gfunc_m256i(__m256i key, __m256i a)
{
    __m256i sbox[8] = {
        _mm256_setr_epi8(
            12, 4, 6, 2,  10, 5,11, 9,  14, 8,13, 7,   0, 3,15, 1,   // 0
            12, 4, 6, 2,  10, 5,11, 9,  14, 8,13, 7,   0, 3,15, 1),  // 0
        _mm256_setr_epi8(
             6, 8, 2, 3,   9,10, 5,12,   1,14, 4, 7,   1,13, 0,15,   // 1
             6, 8, 2, 3,   9,10, 5,12,   1,14, 4, 7,   1,13, 0,15),  // 1
        _mm256_setr_epi8(
            11, 3, 5, 8,  2, 15,10,13,  14, 1, 7, 4,  12, 9, 6, 0,   // 2
            11, 3, 5, 8,  2, 15,10,13,  14, 1, 7, 4,  12, 9, 6, 0),  // 2
        _mm256_setr_epi8(
            12, 8, 2, 1,  13, 4,15, 6,   7, 0,10, 5,   3,14, 9,11,   // 3
            12, 8, 2, 1,  13, 4,15, 6,   7, 0,10, 5,   3,14, 9,11),  // 3
        _mm256_setr_epi8(
             7,15, 5,10,   8, 1, 6,13,   0, 9, 3,14,  11, 4, 2,12,   // 4
             7,15, 5,10,   8, 1, 6,13,   0, 9, 3,14,  11, 4, 2,12),  // 4
        _mm256_setr_epi8(
             5,13,15, 6,   9, 2,12,10,  11, 7, 8, 1,   4, 3,14, 0,   // 5
             5,13,15, 6,   9, 2,12,10,  11, 7, 8, 1,   4, 3,14, 0),  // 5
        _mm256_setr_epi8(
             8,14, 2, 5,   6, 9, 1,12,  15, 4,11, 0,  13,10, 3, 7,   // 6
             8,14, 2, 5,   6, 9, 1,12,  15, 4,11, 0,  13,10, 3, 7),  // 6
        _mm256_setr_epi8(
             1, 7,14,13,   0, 5, 8, 3,   4,15,10, 6,   9,12,11, 2,   // 7
             1, 7,14,13,   0, 5, 8, 3,   4,15,10, 6,   9,12,11, 2)   // 7
    };
    __m256i x = _mm256_add_epi32(a, key), out;
    APPLY_SBOX_LO_TOOUT(0x0000000F, 0xFFFFFF00, 0);
    APPLY_SBOX_HI(0x000000F0, 0xFFFFFF00, 1);
    APPLY_SBOX_LO(0x00000F00, 0xFFFF00FF, 2);
    APPLY_SBOX_HI(0x0000F000, 0xFFFF00FF, 3);
    APPLY_SBOX_LO(0x000F0000, 0xFF00FFFF, 4);
    APPLY_SBOX_HI(0x00F00000, 0xFF00FFFF, 5);
    APPLY_SBOX_LO(0x0F000000, 0x00FFFFFF, 6);
    APPLY_SBOX_HI(0xF0000000, 0x00FFFFFF, 7);
    return mm256_roti_epi32_def(out, 11);
}

static inline void magma_round_m256i(__m256i *a1, __m256i *a0, uint32_t key)
{
    __m256i t = _mm256_xor_si256(*a1, gfunc_m256i(_mm256_set1_epi32(key), *a0));
    *a1 = *a0;
    *a0 = t;
}

#endif


static void MagmaVecState_encrypt(MagmaVecState *obj)
{
#ifdef MAGMA_VEC_ENABLED    
    // Load counters
    __m256i a1 = Vector256_to_m256i(&(obj->ctr_a1));
    __m256i a0 = Vector256_to_m256i(&(obj->ctr_a0));
    // CBC mode: XOR input (counter) with the previous input
    if (obj->mode == MAGMA_CBC) {
        __m256i out_a1 = Vector256_to_m256i(&(obj->out_a1));
        __m256i out_a0 = Vector256_to_m256i(&(obj->out_a0));
        a1 = _mm256_xor_si256(a1, out_a1);
        a0 = _mm256_xor_si256(a0, out_a0);
    }
    // Encryption rounds
    for (int i = 0; i < 3; i++) {
        magma_round_m256i(&a1, &a0, obj->key.w32[0]);
        magma_round_m256i(&a1, &a0, obj->key.w32[1]);
        magma_round_m256i(&a1, &a0, obj->key.w32[2]);
        magma_round_m256i(&a1, &a0, obj->key.w32[3]);
        magma_round_m256i(&a1, &a0, obj->key.w32[4]);
        magma_round_m256i(&a1, &a0, obj->key.w32[5]);
        magma_round_m256i(&a1, &a0, obj->key.w32[6]);
        magma_round_m256i(&a1, &a0, obj->key.w32[7]);
    }
    magma_round_m256i(&a1, &a0, obj->key.w32[7]);
    magma_round_m256i(&a1, &a0, obj->key.w32[6]);
    magma_round_m256i(&a1, &a0, obj->key.w32[5]);
    magma_round_m256i(&a1, &a0, obj->key.w32[4]);
    magma_round_m256i(&a1, &a0, obj->key.w32[3]);
    magma_round_m256i(&a1, &a0, obj->key.w32[2]);
    magma_round_m256i(&a1, &a0, obj->key.w32[1]);
    magma_round_m256i(&a1, &a0, obj->key.w32[0]);
    // Save the encrypted counters
    Vector256_from_m256i(&(obj->out_a1), a1);
    Vector256_from_m256i(&(obj->out_a0), a0);
#else
    (void) obj;
#endif
}


/**
 * @brief Increase internal counters. There are 8 64-bit counters in the AVX2
 * version of MAGMA based PRNG.
 */
static inline void MagmaVecState_inc_ctr(MagmaVecState *obj)
{
    for (int i = 0; i < 8; i++) {
        obj->ctr_a0.w32[i] += 8;
    }
    // 32-bit counters overflow: increment the upper halves
    if (obj->ctr_a0.w32[0] == 0) {
        for (int i = 0; i < 8; i++) {
            obj->ctr_a1.w32[i]++;
        }
    }
}


/**
 * @brief PRNG based on 8 copies of MAGMA block cipher. Uses internal buffers
 * for saving blocks between calls.
 */
static inline uint64_t get_bits_vector_raw(void *state)
{
    MagmaVecState *obj = state;
    if (obj->pos >= 8) {
        // Buffer is empty: encrypt counters and then increment them
        MagmaVecState_encrypt(obj);
        MagmaVecState_inc_ctr(obj);
        obj->pos = 0;
    }
    // Generate 64-bit output
    uint64_t a0_out = obj->out_a0.w32[obj->pos];
    uint64_t a1_out = obj->out_a1.w32[obj->pos];
    obj->pos++;
    return (a0_out << 32) | a1_out;
}

MAKE_GET_BITS_WRAPPERS(vector)


#ifdef MAGMA_VEC_ENABLED    

/**
 * @brief Test for the \f$ g[k](x) \f$ function. Based on test vectors
 * from GOST.
 * @details
 *
 *     g[87654321](fedcba98) = fdcbc20c
 *     g[fdcbc20c](87654321) = 7e791a4b
 *     g[7e791a4b](fdcbc20c) = c76549ec
 *     g[c76549ec](7e791a4b) = 9791c849
 */
static int test_gfunc(const CallerAPI *intf)
{
    static const Vector256 gfunc_ink = {
        .w32 = {
            0x87654321, 0xfdcbc20c, 0x7e791a4b, 0xc76549ec,
            0x87654321, 0xfdcbc20c, 0x7e791a4b, 0xc76549ec
        }
    };

    static const Vector256 gfunc_ina = {
        .w32 = {
            0xfedcba98, 0x87654321, 0xfdcbc20c, 0x7e791a4b,
            0xfedcba98, 0x87654321, 0xfdcbc20c, 0x7e791a4b
        }
    };

    static const Vector256 gfunc_ref = {
        .w32 = {
            0xfdcbc20c, 0x7e791a4b, 0xc76549ec, 0x9791c849,
            0xfdcbc20c, 0x7e791a4b, 0xc76549ec, 0x9791c849
        }
    };
    Vector256 gfunc_out;
    intf->printf("----- gfunc[k](x) test -----\nk:\n");
    Vector256_print(&gfunc_ink, intf);
    intf->printf("x:\n");
    Vector256_print(&gfunc_ina, intf);
    intf->printf("gfunc[k](x)\n");
    Vector256_from_m256i(&gfunc_out, gfunc_m256i(
        Vector256_to_m256i(&gfunc_ink),
        Vector256_to_m256i(&gfunc_ina)));
    intf->printf("Output:\n");
    Vector256_print(&gfunc_out, intf);
    intf->printf("Reference vector:\n");
    Vector256_print(&gfunc_ref, intf);
    intf->printf("\n");

    for (int i = 0; i < 8; i++) {
        if (gfunc_out.w32[i] != gfunc_ref.w32[i]) {
            return 0;
        }
    }
    return 1;
}

static void test_collectw16(const CallerAPI *intf)
{
    static const Vector256 in = {
        .w32 = {
            0x03020100, 0x07060504, 0x0B0A0908, 0x0F0E0D0C,
            0x13121110, 0x17161514, 0x1B1A1918, 0x1F1E1D1C
        }
    };
    Vector256 out;
    Vector256_from_m256i(&out, collect_w16(Vector256_to_m256i(&in)));
    intf->printf("----- test_collectw16 -----\n");
    intf->printf("Input vector:\n");
    Vector256_print(&in, intf);
    intf->printf("After collectw16:\n");
    Vector256_print(&out, intf);
    intf->printf("After uncollect_w16:\n");
    Vector256_from_m256i(&out, uncollect_w16(Vector256_to_m256i(&out)));
    Vector256_print(&out, intf);    
    intf->printf("\n");
}

#endif

static int run_self_test_vector(const CallerAPI *intf)
{
#ifdef MAGMA_VEC_ENABLED
    static const uint32_t key[] = {
        0xffeeddcc, 0xbbaa9988, 0x77665544, 0x33221100,
        0xf0f1f2f3, 0xf4f5f6f7, 0xf8f9fafb, 0xfcfdfeff
    };
    // Only the first ctr-u_ref pair is obtained from the original GOST.
    // Other pairs were generated by means of this (non-vectorized) MAGMA
    // implementation.
    static const uint64_t ctr_in[8] = {
        0xfedcba9876543210, 0x243F69A25B093B12, 0x24C5B22658595D69, 0x0000000000000000,
        0x0123456789ABCDEF, 0xB7E151628AED2A6B, 0xDEADBEEFDEADBEEF, 0xFFFFFFFFFFFFFFFF
    };
    static const uint64_t u_ref[8] = {
        0x4ee901e5c2d8ca3d, 0x55DAEE31ED87E6F7, 0xB644E51E09B20B3E, 0x1BB3E0C407A59322,
        0xA6CB0CB94195EA34, 0x13F900FFCBEEB4FE, 0x9E57E39D28EAC91A, 0x503C956F1519A1A3
    };
    intf->printf("----- Vectorized version internal self-test -----\n");
    // Tests for some internal functions
    test_collectw16(intf);
    if (!test_gfunc(intf)) {
        intf->printf("test_gfunc: failed\n");
        return 0;
    } else {
        intf->printf("test_gfunc: success\n");
    }
    // Tests for encryption procedure
    intf->printf("----- test_get_bits_raw ----\n");
    MagmaVecState *obj = intf->malloc(sizeof(MagmaVecState));
    MagmaVecState_init(obj, key);
    for (int i = 0; i < 8; i++) {
        obj->ctr_a0.w32[i] = (uint32_t) ctr_in[i];
        obj->ctr_a1.w32[i] = ctr_in[i] >> 32;
    }
    uint64_t u;
    int is_ok = 1;
    intf->printf("%18s %18s %18s\n", "In", "Out", "Ref");
    for (int i = 0; i < 8; i++) {
        u = get_bits_vector_raw(obj);
        intf->printf("0x%16.16llX 0x%16.16llX 0x%16.16llX\n",
            ctr_in[i], u, u_ref[i]);
        if (u != u_ref[i]) is_ok = 0;
    }
    intf->free(obj);
    return is_ok;
#else
    intf->printf("----- Vectorized version internal self-test -----\n");
    intf->printf("----- Not implemented\n");
    return 1;
#endif
};


//////////////////////
///// Interfaces /////
//////////////////////


static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    is_ok = is_ok & run_self_test_scalar(intf);
    is_ok = is_ok & run_self_test_vector(intf);
    return is_ok;
}


static const char description[] =
"PRNG based on the MAGMA-GOSTR34.12-2015 block cipher with 64-bit block size.\n"
"Operation modes, i.e. supported param values:\n"
"  scalar-ctr - scalar version in the CTR (counter) mode\n"
"  vector-ctr - vectorized (AVX2) version in the CTR (counter) mode\n"
"  vector-cbc - vectorized (AVX2) version in the CBC mode\n"
"The CTR versions fail 64-bit birthday paradox test\n";


static inline void *create(const CallerAPI *intf)
{
    (void) intf;
    return NULL;
}

int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 64;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "scalar-ctr") || !intf->strcmp(param, "")) {
        gi->name = "MAGMA-GOSTR34.12-2015:scalar-ctr";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "vector-ctr")) {
        gi->name = "MAGMA-GOSTR34.12-2015:vector-ctr";
        gi->create = create_vector_ctr;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else if (!intf->strcmp(param, "vector-cbc")) {
        gi->name = "MAGMA-GOSTR34.12-2015:vector-cbc";
        gi->create = create_vector_cbc;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else {
        gi->name = "MAGMA-GOSTR34.12-2015:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
