// https://dl.acm.org/doi/10.1145/2388576.2388595
// https://meganorm.ru/Data2/1/4293732/4293732907.pdf
// https://tc26.ru/standard/gost/GOST_R_3412-2015.pdf

#include "smokerand/cinterface.h"

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


PRNG_CMODULE_PROLOG


typedef union {
    uint32_t w32[8];
    uint64_t w64[4];
} Vector256;


typedef struct {
    Vector256 key; ///< 256-bit encryption key (PRNG seed)
    Vector256 ctr_a0; ///< Lower halves of 8 counters
    Vector256 ctr_a1; ///< Upper halves of 8 counters
    Vector256 out_a0; ///< Buffer for output data (encrypted counters)
    Vector256 out_a1; ///< Buffer for output data (encrypted counters)
    int pos; ///< Current position in the output buffer
} MagmaState;


void MagmaState_init(MagmaState *obj, const uint32_t *key)
{
    for (int i = 0; i < 8; i++) {
        obj->key.w32[i] = key[i];
        obj->ctr_a0.w32[i] = i;
        obj->ctr_a1.w32[i] = 0;
    }
    obj->pos = 8;
}

static void *create(const CallerAPI *intf)
{
    MagmaState *obj = intf->malloc(sizeof(MagmaState));
    uint32_t key[8];
    for (int i = 0; i < 4; i++) {
        uint64_t seed = intf->get_seed64();
        key[2*i] = seed >> 32;
        key[2*i + 1] = (uint32_t) seed;
    }
    MagmaState_init(obj, key);
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
    for (int i = 0; i < 8; i++) {
        intf->printf("0x%8.8X ", obj->w32[i]);
    }
}


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
    __m256i x = _mm256_add_epi32(a, key);
    __m256i out = _mm256_set1_epi32(0);
    (void) sbox;
    APPLY_SBOX_LO(0x0000000F, 0xFFFFFF00, 0);
    APPLY_SBOX_HI(0x000000F0, 0xFFFFFF00, 1);
    APPLY_SBOX_LO(0x00000F00, 0xFFFF00FF, 2);
    APPLY_SBOX_HI(0x0000F000, 0xFFFF00FF, 3);
    APPLY_SBOX_LO(0x000F0000, 0xFF00FFFF, 4);
    APPLY_SBOX_HI(0x00F00000, 0xFF00FFFF, 5);
    APPLY_SBOX_LO(0x0F000000, 0x00FFFFFF, 6);
    APPLY_SBOX_HI(0xF0000000, 0x00FFFFFF, 7);
    return mm256_roti_epi32_def(out, 11);
}

/*
static inline __m256i gfunc_m256i(__m256i k, __m256i a)
{
    __m256i x = tfunc_m256i(_mm256_add_epi32(a, k));
    return mm256_roti_epi32_def(x, 11);
}
*/

static inline void magma_round_m256i(__m256i *a1, __m256i *a0, uint32_t key)
{
    __m256i t = _mm256_xor_si256(*a1, gfunc_m256i(_mm256_set1_epi32(key), *a0));
    *a1 = *a0;
    *a0 = t;
}


static void magma_rounds(MagmaState *obj, __m256i *a1, __m256i *a0)
{
    for (int i = 0; i < 3; i++) {
        magma_round_m256i(a1, a0, obj->key.w32[0]);
        magma_round_m256i(a1, a0, obj->key.w32[1]);
        magma_round_m256i(a1, a0, obj->key.w32[2]);
        magma_round_m256i(a1, a0, obj->key.w32[3]);
        magma_round_m256i(a1, a0, obj->key.w32[4]);
        magma_round_m256i(a1, a0, obj->key.w32[5]);
        magma_round_m256i(a1, a0, obj->key.w32[6]);
        magma_round_m256i(a1, a0, obj->key.w32[7]);
    }
    magma_round_m256i(a1, a0, obj->key.w32[7]);
    magma_round_m256i(a1, a0, obj->key.w32[6]);
    magma_round_m256i(a1, a0, obj->key.w32[5]);
    magma_round_m256i(a1, a0, obj->key.w32[4]);
    magma_round_m256i(a1, a0, obj->key.w32[3]);
    magma_round_m256i(a1, a0, obj->key.w32[2]);
    magma_round_m256i(a1, a0, obj->key.w32[1]);
    magma_round_m256i(a1, a0, obj->key.w32[0]);
}


static inline uint64_t get_bits_raw(void *state)
{
    MagmaState *obj = state;
    if (obj->pos >= 8) {
        __m256i a1 = Vector256_to_m256i(&(obj->ctr_a1));
        __m256i a0 = Vector256_to_m256i(&(obj->ctr_a0));
        magma_rounds(obj, &a1, &a0);
        Vector256_from_m256i(&(obj->out_a1), a1);
        Vector256_from_m256i(&(obj->out_a0), a0);
        // Increase counter
        for (int i = 0; i < 8; i++) {
            if (obj->ctr_a0.w32[i] += 8) obj->ctr_a1.w32[i] += 1;
        }
        obj->pos = 0;
    }
    uint64_t a0_out = obj->out_a0.w32[obj->pos];
    uint64_t a1_out = obj->out_a1.w32[obj->pos];
    obj->pos++;
    return (a0_out << 32) | a1_out;
}





static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t key[] = {
        0xffeeddcc, 0xbbaa9988, 0x77665544, 0x33221100,
        0xf0f1f2f3, 0xf4f5f6f7, 0xf8f9fafb, 0xfcfdfeff
    };

/*
t(fdb97531) = 2a196f34,
t(2a196f34) = ebd9f03a,
t(ebd9f03a) = b039bb3d,
t(b039bb3d) = 68695433.
*/

/*
    static const Vector256 tfunc_in = {
        .w32 = {
            0xfdb97531, 0x2a196f34, 0xebd9f03a, 0xb039bb3d,
            0xfdb97531, 0x2a196f34, 0xebd9f03a, 0xb039bb3d
        }
    };
    Vector256 tfunc_out, gfunc_out;

    Vector256_print(&tfunc_in, intf);
    intf->printf("\n");
    Vector256_from_m256i(&tfunc_out, tfunc_m256i(Vector256_to_m256i(&tfunc_in)));
    Vector256_print(&tfunc_out, intf);
    intf->printf("\n");
*/

/*
g[87654321](fedcba98) = fdcbc20c,
g[fdcbc20c](87654321) = 7e791a4b,
g[7e791a4b](fdcbc20c) = c76549ec,
g[c76549ec](7e791a4b) = 9791c849.
*/

    Vector256 gfunc_out;
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
    Vector256_from_m256i(&gfunc_out, gfunc_m256i(
        Vector256_to_m256i(&gfunc_ink),
        Vector256_to_m256i(&gfunc_ina)));
    Vector256_print(&gfunc_out, intf);
    intf->printf("\n");



///////////////////////////////


    MagmaState *obj = intf->malloc(sizeof(MagmaState));
    MagmaState_init(obj, key);
    for (int i = 0; i < 8; i++) {
        obj->ctr_a0.w32[i] = 0x76543210;
        obj->ctr_a1.w32[i] = 0xfedcba98;
    }
    static const uint64_t u_ref = 0x4ee901e5c2d8ca3d;
    uint64_t u = get_bits_raw(obj);
    intf->printf("Out = 0x%llX; ref = 0x%llX", u, u_ref);
    intf->free(obj);
    return (u == u_ref);
};

MAKE_UINT64_PRNG("MAGMA-GOST89", run_self_test)
