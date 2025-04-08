/**
 * @file magma_avx.c
 * @brief CSPRNG based on "Magma" from GOST R 34.12-2015 (version for AVX2)
 * @details "Magma" from GOST R 34.12-2015 is a 64-bit block cipher developed
 * in USSR. It is a Feistel cipher that uses 8 4x4 S-boxes. This version
 * of algorithm is designed for x86-64 processors with AVX2 instructions set
 * and 3-4 times faster than the non-vectorized version. However it is still
 * slow, around 7 cpb.
 *
 * Reduced round version testing:
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
#include "smokerand/x86exts.h"

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
 * @brief MAGMA-AVX-GOSTR34.12-2015 CSPRNG state.
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

static void *create(const CallerAPI *intf)
{
    // Get operation mode
    MagmaMode mode;
    const char *mode_name = intf->get_param();
    if (!intf->strcmp(mode_name, "ctr") || !intf->strcmp(mode_name, "")) {
        mode = MAGMA_CTR;
        intf->printf("Operation mode: ctr\n");
    } else if (!intf->strcmp(mode_name, "cbc")) {
        mode = MAGMA_CBC;
        intf->printf("Operation mode: cbc\n");
    } else {
        intf->printf("Unknown operation mode '%s' (ctr or cbc are supported)",
            mode_name);
        return NULL;
    }
    // Create PRNG example
    MagmaVecState *obj = intf->malloc(sizeof(MagmaVecState));
    uint32_t key[8];
    for (int i = 0; i < 4; i++) {
        uint64_t seed = intf->get_seed64();
        key[2*i] = seed >> 32;
        key[2*i + 1] = (uint32_t) seed;
    }
    MagmaVecState_init(obj, key);
    obj->mode = mode;
    return (void *) obj;
}



static inline __m256i Vector256_to_m256i(const Vector256 *obj)
{
    return _mm256_loadu_si256((__m256i *) obj->w32);
}

static inline void Vector256_from_m256i(Vector256 *obj, __m256i x)
{
    _mm256_storeu_si256((__m256i *) obj->w32, x);
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


static void MagmaVecState_encrypt(MagmaVecState *obj)
{
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
static inline uint64_t get_bits_raw(void *state)
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


static int run_self_test(const CallerAPI *intf)
{
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
        u = get_bits_raw(obj);
        intf->printf("0x%16.16llX 0x%16.16llX 0x%16.16llX\n",
            ctr_in[i], u, u_ref[i]);
        if (u != u_ref[i]) is_ok = 0;
    }
    intf->free(obj);
    return is_ok;
};

MAKE_UINT64_PRNG("MAGMA-AVX-GOSTR34.12-2015", run_self_test)
