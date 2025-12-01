/**
 * @file threefish1024.c
 * @brief An implementation of pseudorandom number generator based on the
 * Threefish-1024 block cipher, a part of Skein hash functoon.
 * @details This module contains both scalar and vectorized implementations
 * of ThreeFish-1024 based PRNG (`c99`, `avx2` paramter values). The vectorized
 * version uses the x86-64 AVX2 instructions set.
 *
 * Both portable and AVX2 versions give an identical output.
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
 * WARNING! This program is designed as a general purpose high quality PRNG
 * for simulations and statistical testing. IT IS NOT DESIGNED FOR ENCRYPTION,
 * KEYS/NONCES GENERATION AND OTHER CRYPTOGRAPHICAL APPLICATION!
 *
 * @copyright Threefish block cipher was developed by Bruce Schneier et al.
 * Threefry algorithm was proposed by J. K. Salmon, M. A. Moraes, R. O. Dror
 * and D. E. Shaw.
 *
 * Some optimization ideas were taken from the public domain code of the
 * optimized Skein function implementaton written by Doug Whiting in 2008.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

#ifdef  __AVX2__
#define TF1024_VEC_ENABLED
#include "smokerand/x86exts.h"
#endif

PRNG_CMODULE_PROLOG

enum {
    ROT0_0 = 24, ROT0_1 = 13, ROT0_2 = 8,  ROT0_3 = 47, ROT0_4 = 8,  ROT0_5 = 17, ROT0_6 = 22, ROT0_7 = 37,
    ROT1_0 = 38, ROT1_1 = 19, ROT1_2 = 10, ROT1_3 = 55, ROT1_4 = 49, ROT1_5 = 18, ROT1_6 = 23, ROT1_7 = 52,
    ROT2_0 = 33, ROT2_1 = 4,  ROT2_2 = 51, ROT2_3 = 13, ROT2_4 = 34, ROT2_5 = 41, ROT2_6 = 59, ROT2_7 = 17,
    ROT3_0 = 5,  ROT3_1 = 20, ROT3_2 = 48, ROT3_3 = 41, ROT3_4 = 47, ROT3_5 = 28, ROT3_6 = 16, ROT3_7 = 25,
    ROT4_0 = 41, ROT4_1 = 9,  ROT4_2 = 37, ROT4_3 = 31, ROT4_4 = 12, ROT4_5 = 47, ROT4_6 = 44, ROT4_7 = 30,
    ROT5_0 = 16, ROT5_1 = 34, ROT5_2 = 56, ROT5_3 = 51, ROT5_4 = 4,  ROT5_5 = 53, ROT5_6 = 42, ROT5_7 = 41,
    ROT6_0 = 31, ROT6_1 = 44, ROT6_2 = 47, ROT6_3 = 46, ROT6_4 = 19, ROT6_5 = 42, ROT6_6 = 44, ROT6_7 = 25,
    ROT7_0 = 9,  ROT7_1 = 48, ROT7_2 = 35, ROT7_3 = 52, ROT7_4 = 23, ROT7_5 = 31, ROT7_6 = 37, ROT7_7 = 20
};


enum {
    TF1024_NWORDS = 16, ///< Number of words per state (DON'T CHANGE!)
    TF1024_NCOPIES = 4, ///< Number of generator copies (DON'T CHANGE!)
    TF1024_NROUNDS = 80 ///< Number of rounds for Threefish-1024
};

#define C240 0x1BD11BDAA9FC1A22U

//////////////////////////////////////////////////////////
///// 

/**
 * @brief Threefish-1024 scalar pseudorandom number generator state, it doesn't
 * rely on SIMD and is cross-platform.
 */
typedef struct {
    uint64_t k[TF1024_NWORDS + 1]; /// Key (+ extra word)
    uint64_t t[3]; ///< Tweak
    uint64_t p[TF1024_NWORDS]; /// Counter ("plain text")
    uint64_t v[TF1024_NWORDS]; /// Output buffer
    unsigned int pos;
} Tf1024State;


#define MIX16_ITER(i0, i1, di) { \
    x[i0] = x[i0] + x[i1]; \
    x[i1] = rotl64(x[i1], di) ^ x[i0]; \
}


#define MIX16(i0,i1,i2,i3, i4,i5,i6,i7, i8,i9,iA,iB, iC,iD,iE,iF, rotid) { \
    MIX16_ITER(i0, i1, ROT##rotid##_0) \
    MIX16_ITER(i2, i3, ROT##rotid##_1) \
    MIX16_ITER(i4, i5, ROT##rotid##_2) \
    MIX16_ITER(i6, i7, ROT##rotid##_3) \
    MIX16_ITER(i8, i9, ROT##rotid##_4) \
    MIX16_ITER(iA, iB, ROT##rotid##_5) \
    MIX16_ITER(iC, iD, ROT##rotid##_6) \
    MIX16_ITER(iE, iF, ROT##rotid##_7) \
}


#define MIX16_HALF0 \
    MIX16( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,0); \
    MIX16( 0, 9, 2,13, 6,11, 4,15,10, 7,12, 3,14, 5, 8, 1,1); \
    MIX16( 0, 7, 2, 5, 4, 3, 6, 1,12,15,14,13, 8,11,10, 9,2); \
    MIX16( 0,15, 2,11, 6,13, 4, 9,14, 1, 8, 5,10,03,12, 7,3); \

#define MIX16_HALF1 \
    MIX16( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 4); \
    MIX16( 0, 9, 2,13, 6,11, 4,15,10, 7,12, 3,14, 5, 8, 1, 5); \
    MIX16( 0, 7, 2, 5, 4, 3, 6, 1,12,15,14,13, 8,11,10, 9, 6); \
    MIX16( 0,15, 2,11, 6,13, 4, 9,14, 1, 8, 5,10,03,12, 7, 7); \

/**
 * @brief Generates round keys for Threefish-1024.
 */
static inline void Tf1024State_key_schedule(Tf1024State *obj, uint64_t *out, size_t s)
{
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        out[i] += obj->k[(s + i) % (TF1024_NWORDS + 1)];
    }
    out[TF1024_NWORDS - 3] += obj->t[s % 3];
    out[TF1024_NWORDS - 2] += obj->t[(s + 1) % 3];
    out[TF1024_NWORDS - 1] += s;
}


#define TF1024_ROUNDS8(s) { \
    Tf1024State_key_schedule(obj, x, (s)); \
    MIX16_HALF0 \
    Tf1024State_key_schedule(obj, x, (s) + 1); \
    MIX16_HALF1 \
}

/**
 * @brief Generate the 1024-bit block for Threefish-1024.
 */
EXPORT void Tf1024State_block(Tf1024State *obj)
{
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        obj->v[i] = obj->p[i];
    }
    uint64_t *x = obj->v;
    TF1024_ROUNDS8(0)  TF1024_ROUNDS8(2)
    TF1024_ROUNDS8(4)  TF1024_ROUNDS8(6)
    TF1024_ROUNDS8(8)  TF1024_ROUNDS8(10)
    TF1024_ROUNDS8(12) TF1024_ROUNDS8(14)
    TF1024_ROUNDS8(16) TF1024_ROUNDS8(18)
    Tf1024State_key_schedule(obj, x, TF1024_NROUNDS / 4);
}

/**
 * @brief Initialized the Threefry/Threefish pseudorandom number generator
 * internal state.
 * @param obj      Generator state to be initialized.
 * @param k        Pointer to the 1024-bit key (seed).
 * @param t        Pointer to the 128-bit tweak.
 */
static void Tf1024State_init(Tf1024State *obj, const uint64_t *k, const uint64_t *t)
{
    obj->k[TF1024_NWORDS] = C240;
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        obj->k[i] = k[i];
        obj->p[i] = 0;
        obj->k[TF1024_NWORDS] ^= obj->k[i];
    }
    obj->t[0] = t[0];
    obj->t[1] = t[1];
    obj->t[2] = t[0] ^ t[1];
    obj->pos = 0;
    Tf1024State_block(obj);
}


static uint64_t get_bits_scalar_raw(void *state)
{
    Tf1024State *obj = state;
    if (obj->pos >= TF1024_NWORDS) {
        obj->p[0]++;
        Tf1024State_block(obj);
        obj->pos = 0;
    }
    return obj->v[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(scalar)

static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    uint64_t key[TF1024_NWORDS], tweak[2] = {0, 0};
    Tf1024State *obj = intf->malloc(sizeof(Tf1024State));
    (void) gi;
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        key[i] = intf->get_seed64();
    }
    Tf1024State_init(obj, key, tweak);
    return obj;
}


///////////////////////////////////////////

typedef union {
    uint64_t u64[TF1024_NCOPIES];
} Tf1024Element;


/**
 * @brief Threefish-1024 AVX2 pseudorandom number generator state, it doesn't
 * rely on SIMD and is cross-platform.
 */
typedef struct {
    uint64_t k[TF1024_NWORDS + 1]; /// Key (+ extra word)
    uint64_t t[3]; ///< Tweak
    Tf1024Element p[TF1024_NWORDS]; /// Counter ("plain text")
    Tf1024Element v[TF1024_NWORDS]; /// Output buffer
    unsigned int pos;
} Tf1024VecState;


#define MIX16V_ITER(i0, i1, di) { \
    x[i0] = _mm256_add_epi64(x[i0], x[i1]); \
    x[i1] = _mm256_xor_si256(mm256_rotl_epi64_def(x[i1], di), x[i0]); \
}

#define MIX16V(i0,i1,i2,i3, i4,i5,i6,i7, i8,i9,iA,iB, iC,iD,iE,iF, rotid) { \
    MIX16V_ITER(i0, i1, ROT##rotid##_0) \
    MIX16V_ITER(i2, i3, ROT##rotid##_1) \
    MIX16V_ITER(i4, i5, ROT##rotid##_2) \
    MIX16V_ITER(i6, i7, ROT##rotid##_3) \
    MIX16V_ITER(i8, i9, ROT##rotid##_4) \
    MIX16V_ITER(iA, iB, ROT##rotid##_5) \
    MIX16V_ITER(iC, iD, ROT##rotid##_6) \
    MIX16V_ITER(iE, iF, ROT##rotid##_7) \
}

#define MIX16V_HALF0 \
    MIX16V( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,0); \
    MIX16V( 0, 9, 2,13, 6,11, 4,15,10, 7,12, 3,14, 5, 8, 1,1); \
    MIX16V( 0, 7, 2, 5, 4, 3, 6, 1,12,15,14,13, 8,11,10, 9,2); \
    MIX16V( 0,15, 2,11, 6,13, 4, 9,14, 1, 8, 5,10,03,12, 7,3); \

#define MIX16V_HALF1 \
    MIX16V( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 4); \
    MIX16V( 0, 9, 2,13, 6,11, 4,15,10, 7,12, 3,14, 5, 8, 1, 5); \
    MIX16V( 0, 7, 2, 5, 4, 3, 6, 1,12,15,14,13, 8,11,10, 9, 6); \
    MIX16V( 0,15, 2,11, 6,13, 4, 9,14, 1, 8, 5,10,03,12, 7, 7); \


#ifdef TF1024_VEC_ENABLED

/**
 * @brief Generates round keys for Threefish-1024.
 */
static inline void Tf1024VecState_key_schedule(Tf1024VecState *obj, __m256i *out, size_t s)
{
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        const uint64_t ks = obj->k[(s + i) % (TF1024_NWORDS + 1)];
        out[i] = _mm256_add_epi64(out[i], _mm256_set1_epi64x((long long) ks));
    }
    out[TF1024_NWORDS - 3] = _mm256_add_epi64(out[TF1024_NWORDS - 3], _mm256_set1_epi64x((long long) obj->t[s % 3]));
    out[TF1024_NWORDS - 2] = _mm256_add_epi64(out[TF1024_NWORDS - 2], _mm256_set1_epi64x((long long) obj->t[(s + 1) % 3]));
    out[TF1024_NWORDS - 1] = _mm256_add_epi64(out[TF1024_NWORDS - 1], _mm256_set1_epi64x((long long) s));
}

#endif

#define TF1024V_ROUNDS8(s) { \
    Tf1024VecState_key_schedule(obj, x, (s)); \
    MIX16V_HALF0 \
    Tf1024VecState_key_schedule(obj, x, (s) + 1); \
    MIX16V_HALF1 \
}


/**
 * @brief Generate the 1024-bit block for Threefish-1024.
 */
EXPORT void Tf1024VecState_block(Tf1024VecState *obj)
{
#ifdef TF1024_VEC_ENABLED
    __m256i x[TF1024_NWORDS];    
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        x[i] = _mm256_loadu_si256((__m256i *) (void *) (obj->p + i));
    }
    TF1024V_ROUNDS8(0)  TF1024V_ROUNDS8(2)
    TF1024V_ROUNDS8(4)  TF1024V_ROUNDS8(6)
    TF1024V_ROUNDS8(8)  TF1024V_ROUNDS8(10)
    TF1024V_ROUNDS8(12) TF1024V_ROUNDS8(14)
    TF1024V_ROUNDS8(16) TF1024V_ROUNDS8(18)
    Tf1024VecState_key_schedule(obj, x, TF1024_NROUNDS / 4);
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        _mm256_storeu_si256((__m256i *) (void *) (obj->v + i), x[i]);
    }
#else
    (void) obj;
#endif
}


/**
 * @brief Initialized the Threefry/Threefish pseudorandom number generator
 * internal state.
 * @param obj      Generator state to be initialized.
 * @param k        Pointer to the 1024-bit key (seed).
 * @param t        Pointer to the 128-bit tweak.
 */
static void Tf1024VecState_init(Tf1024VecState *obj, const uint64_t *k, const uint64_t *t)
{
    // Key and state
    obj->k[TF1024_NWORDS] = C240;
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        obj->k[i] = k[i];
        obj->k[TF1024_NWORDS] ^= obj->k[i];
    }
    // Tweak
    obj->t[0] = t[0];
    obj->t[1] = t[1];
    obj->t[2] = t[0] ^ t[1];
    // State and counters
    for (size_t i = 0; i < TF1024_NCOPIES; i++) {
        obj->p[0].u64[i] = i; // 64-bit counter
        for (size_t j = 1; j < TF1024_NWORDS; j++) {
            obj->p[j].u64[i] = 0;
        }
    }
    obj->pos = 0;
    Tf1024VecState_block(obj);
}


static inline void Tf1024VecState_inc_counter(Tf1024VecState *obj)
{
    for (size_t i = 0; i < TF1024_NCOPIES; i++) {
        obj->p[0].u64[i] += TF1024_NCOPIES;
    }    
}

static uint64_t get_bits_vector_raw(void *state)
{
    Tf1024VecState *obj = state;
    if (obj->pos >= TF1024_NWORDS * TF1024_NCOPIES) {
        Tf1024VecState_inc_counter(obj);
        Tf1024VecState_block(obj);
        obj->pos = 0;
    }
    const size_t i = (obj->pos & 0xF), j = (obj->pos >> 4);
    const uint64_t x = obj->v[i].u64[j];
    obj->pos++;
    return x;
}

MAKE_GET_BITS_WRAPPERS(vector)

static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef TF1024_VEC_ENABLED
    uint64_t key[TF1024_NWORDS], tweak[2] = {0, 0};
    Tf1024VecState *obj = intf->malloc(sizeof(Tf1024VecState));
    (void) gi;
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        key[i] = intf->get_seed64();
    }
    Tf1024VecState_init(obj, key, tweak);
    return obj;
#else
    (void) Tf1024VecState_init;
    (void) gi;
    intf->printf("Not implemented\n");
    return NULL;
#endif
}



//////////////////////////

/**
 * @brief An internal self-test based on the original test vectors from
 * Skein function reference implementation. See the next reference:
 *
 * - https://www.schneier.com/academic/skein/threefish/
 */
static int run_self_test(const CallerAPI *intf)
{
    Tf1024State *obj = intf->malloc(sizeof(Tf1024State));
    static const uint64_t key_1[TF1024_NWORDS] = {
        0, 0, 0, 0,  0, 0, 0, 0,
        0, 0, 0, 0,  0, 0, 0, 0};
    static uint64_t tweak_1[2] = {0, 0};
    static const uint64_t ref_1[TF1024_NWORDS] = {
        0x04B3053D0A3D5CF0, 0x0136E0D1C7DD85F7, 0x067B212F6EA78A5C, 0x0DA9C10B4C54E1C6,
        0x0F4EC27394CBACF0, 0x32437F0568EA4FD5, 0xCFF56D1D7654B49C, 0xA2D5FB14369B2E7B,
        0x540306B460472E0B, 0x71C18254BCEA820D, 0xC36B4068BEAF32C8, 0xFA4329597A360095,
        0xC4A36C28434A5B9A, 0xD54331444B1046CF, 0xDF11834830B2A460, 0x1E39E8DFE1F7EE4F
    };


    static const uint64_t tweak_2[2] = {0x0706050403020100, 0x0F0E0D0C0B0A0908};
    static const uint64_t key_2[16] = {
        0x1716151413121110, 0x1F1E1D1C1B1A1918, 0x2726252423222120, 0x2F2E2D2C2B2A2928,
        0x3736353433323130, 0x3F3E3D3C3B3A3938, 0x4746454443424140, 0x4F4E4D4C4B4A4948,
        0x5756555453525150, 0x5F5E5D5C5B5A5958, 0x6766656463626160, 0x6F6E6D6C6B6A6968,
        0x7776757473727170, 0x7F7E7D7C7B7A7978, 0x8786858483828180, 0x8F8E8D8C8B8A8988
    };
    static const uint64_t ref_2[16] = {
        0xB0C33CD7DB4D65A6, 0xBC49A85A1077D75D, 0x6855FCAFEA7293E4, 0x1C5385AB1B7754D2,
        0x30E4AAFFE780F794, 0xE1BBEE708CAFD8D5, 0x9CA837B7423B0F76, 0xBD1403670D4963B3,
        0x451F2E3CE61EA48A, 0xB360832F9277D4FB, 0x0AAFC7A65E12D688, 0xC8906E79016D05D7,
        0xB316570A15F41333, 0x74E98A2869F5D50E, 0x57CE6F9247432BCE, 0xDE7CDD77215144DE
    };
    int is_ok = 1;

    intf->printf("Testing the reference scalar implementation...\n");
    Tf1024State_init(obj, key_1, tweak_1);
    Tf1024State_block(obj);
    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        if (obj->v[i] != ref_1[i]) {
            is_ok = 0;
        }
    }

    Tf1024State_init(obj, key_2, tweak_2);
    obj->p[0]  = 0xF8F9FAFBFCFDFEFF; obj->p[1]  = 0xF0F1F2F3F4F5F6F7;
    obj->p[2]  = 0xE8E9EAEBECEDEEEF; obj->p[3]  = 0xE0E1E2E3E4E5E6E7;
    obj->p[4]  = 0xD8D9DADBDCDDDEDF; obj->p[5]  = 0xD0D1D2D3D4D5D6D7;
    obj->p[6]  = 0xC8C9CACBCCCDCECF; obj->p[7]  = 0xC0C1C2C3C4C5C6C7;
    obj->p[8]  = 0xB8B9BABBBCBDBEBF; obj->p[9]  = 0xB0B1B2B3B4B5B6B7;
    obj->p[10] = 0xA8A9AAABACADAEAF; obj->p[11] = 0xA0A1A2A3A4A5A6A7; 
    obj->p[12] = 0x98999A9B9C9D9E9F; obj->p[13] = 0x9091929394959697;
    obj->p[14] = 0x88898A8B8C8D8E8F; obj->p[15] = 0x8081828384858687;
    Tf1024State_block(obj);

    for (size_t i = 0; i < TF1024_NWORDS; i++) {
        if (obj->v[i] != ref_2[i]) {
            is_ok = 0;
        }
    }



#ifdef TF1024_VEC_ENABLED
    intf->printf("Comparison of scalar and vector implementations...\n");
    Tf1024State_init(obj, key_2, tweak_2);
    Tf1024VecState *vecobj = intf->malloc(sizeof(Tf1024VecState));
    Tf1024VecState_init(vecobj, key_2, tweak_2);
    for (long i = 0; i < 10000000; i++) {
        const uint64_t u_sc = get_bits_scalar_raw(obj);
        const uint64_t u_vec = get_bits_vector_raw(vecobj);
        if (u_sc != u_vec) {
            is_ok = 0;
            intf->printf("%ld: sc = %" PRIX64 " vec = %" PRIX64 "\n",
                i, u_sc, u_vec);
            break;
        }
    }
    intf->free(vecobj);    
#endif
    intf->free(obj);
    return is_ok;
}


static inline void *create(const CallerAPI *intf)
{
    intf->printf("Not implemented\n");
    return NULL;
}



static const char description[] =
"A counter based PRNG based on the ThreeFish-1024 block cipher.\n"
"The next param values are supported:\n"
"  c99  - portable version, default. Performance is around 4.0-4.5 cpb.\n"
"  avx2 - AVX2 version. Performance is around 1.3 cpb.\n";


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
        gi->name = "ThreeFish1024:c99";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "avx2")) {
        gi->name = "ThreeFish1024:avx2";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
#ifndef TF1024_VEC_ENABLED
        intf->printf("Not implemented\n");
        return 0;
#endif
    } else {
        gi->name = "ThreeFish1024:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
        return 0;
    }
    return 1;
}
