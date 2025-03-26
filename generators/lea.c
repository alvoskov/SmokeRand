/**
 * @file lea.c
 * @brief PRNG based on the LEA128 block cipher in the CTR mode.
 * @details Test vectors for LEA128 (as 32-bit words):
 *
 *     KEY:    0x3c2d1e0f, 0x78695a4b, 0xb4a59687, 0xf0e1d2c3
 *     RKEY0:  0x003a0fd4, 0x02497010, 0x194f7db1, 0x02497010, 0x090d0883, 0x02497010
 *     RKEY23: 0x0bf6adba, 0xdf69029d, 0x5b72305a, 0xdf69029d, 0xcb47c19f, 0xdf69029d
 *     INPUT:  0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c
 *     OUTPUT: 0x354ec89f, 0x18c6c628, 0xa7c73255, 0xfd8b6404
 *
 * References:
 *
 * 1. Hong D., Lee J.K., Kim D.C., Kwon D., Ryu K.H., Lee D.G. LEA: A 128-Bit
 *    Block Cipher for Fast Encryption on Common Processors. In: Kim Y.,
 *    Lee H., Perrig A. (eds) Information Security Applications (2014).
 *    WISA 2013. Lecture Notes in Computer Science(), vol 8267. Springer, Cham.
 *    https://doi.org/10.1007/978-3-319-05149-9_1
 * 2. ISO/IEC 29192-2:2019(E) International Standard. Informational security -
 *    - Lightweight cryptograpy. Part 2. Block ciphers.
 * 3. KS X 3246, 128-bit block cipher LEA (in Korean)
 *    https://www.rra.go.kr/ko/reference/kcsList_view.do?nb_seq=1923&nb_type=6
 * 4. https://github.com/weidai11/cryptopp/blob/master/TestVectors/lea.txt
 *
 * Tests:
 * - 8 rounds - fails `express` battery.
 * - 9 rounds - passes `express`, `default`, fails `full`
 * - 10 rounds - passes `full` battery.
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define LEA_NROUNDS 24
#define LEA_RK_ALIGN 4
#define LEA_NCOPIES 16

#ifdef  __AVX2__
#define LEA_VEC_ENABLED
#endif

#ifdef LEA_VEC_ENABLED
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif


typedef struct {
    void (*block_func)(void *);
    int pos;
    int bufsize;
    uint32_t *out;
} LeaInterface;

typedef struct {
    LeaInterface intf;
    uint32_t ctr[4]; ///< Counter (plaintext)
    uint32_t out[4]; ///< Output buffer (ciphertext)
    uint32_t rk[LEA_NROUNDS * LEA_RK_ALIGN]; ///< Round keys (0,2,4) | [1,3,5]
} LeaState;

typedef struct {
    LeaInterface intf;
    uint32_t ctr[4 * LEA_NCOPIES]; ///< Counter (plaintext)
    uint32_t out[4 * LEA_NCOPIES]; ///< Output buffer (ciphertext)
    uint32_t rk[LEA_NROUNDS * LEA_RK_ALIGN]; ///< Round keys (0,2,4) | [1,3,5]
} LeaVecState;



void lea128_fill_round_keys(uint32_t *rk, const uint32_t *key)
{
    static const uint32_t delta[8] = {
        0xc3efe9db, 0x44626b02, 0x79e27c8a, 0x78df30ec,
        0x715ea49e, 0xc785da0a, 0xe04ef22a, 0xe5c40957
    };
    uint32_t t[4];
    for (int i = 0; i < 4; i++) {
        t[i] = key[i];
    }
    for (int i = 0; i < LEA_NROUNDS; i++) {
        uint32_t di = delta[i & 3];
        int rk_ind = i * LEA_RK_ALIGN;
        t[0] = rotl32(t[0] + rotl32(di, i), 1);
        t[1] = rotl32(t[1] + rotl32(di, i + 1), 3);
        t[2] = rotl32(t[2] + rotl32(di, i + 2), 6);
        t[3] = rotl32(t[3] + rotl32(di, i + 3), 11);
        rk[rk_ind] = t[0]; rk[rk_ind + 1] = t[2]; rk[rk_ind + 2] = t[3];
        rk[rk_ind + 3] = t[1];
    }
}

void LeaState_block(LeaState *obj)
{
    uint32_t c[4];
    uint32_t *rk = obj->rk;
    for (int i = 0; i < 4; i++) {
        c[i] = obj->ctr[i];
    }
    for (int i = 0; i < LEA_NROUNDS; i++) {
        uint32_t c0_old = c[0];
        c[0] = rotl32((c[0] ^ rk[0]) + (c[1] ^ rk[3]), 9);
        c[1] = rotr32((c[1] ^ rk[1]) + (c[2] ^ rk[3]), 5);
        c[2] = rotr32((c[2] ^ rk[2]) + (c[3] ^ rk[3]), 3);
        c[3] = c0_old;
        rk += LEA_RK_ALIGN;
    }
    for (int i = 0; i < 4; i++) {
        obj->out[i] = c[i];
    }
}

#ifdef LEA_VEC_ENABLED
/**
 * @brief Vectorized "rotate left" instruction for vector of 32-bit values.
 */
static inline __m256i rotl32_vec(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi32(in, r), _mm256_srli_epi32(in, 32 - r));
}

/**
 * @brief Vectorized "rotate right" instruction for vector of 32-bit values.
 */
static inline __m256i rotr32_vec(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi32(in, 32 - r), _mm256_srli_epi32(in, r));
}
#endif


#ifdef LEA_VEC_ENABLED
static inline void leavec_load_ctr(__m256i *c, const uint32_t *ctr)
{
    c[0] = _mm256_loadu_si256((__m256i *) ctr);
    c[1] = _mm256_loadu_si256((__m256i *) (ctr + LEA_NCOPIES));
    c[2] = _mm256_loadu_si256((__m256i *) (ctr + 2*LEA_NCOPIES));
    c[3] = _mm256_loadu_si256((__m256i *) (ctr + 3*LEA_NCOPIES));
}

static inline void leavec_store_out(uint32_t *out, const __m256i *c)
{
    _mm256_storeu_si256((__m256i *) out,                   c[0]);
    _mm256_storeu_si256((__m256i *) (out + LEA_NCOPIES),   c[1]);
    _mm256_storeu_si256((__m256i *) (out + 2*LEA_NCOPIES), c[2]);
    _mm256_storeu_si256((__m256i *) (out + 3*LEA_NCOPIES), c[3]);
}

static inline void leavec_round(__m256i *c, const __m256i *rka, __m256i rkb)
{
    __m256i c0_old = c[0];
    c[0] = rotl32_vec(_mm256_add_epi32(_mm256_xor_si256(c[0], rka[0]), _mm256_xor_si256(c[1], rkb)), 9);
    c[1] = rotr32_vec(_mm256_add_epi32(_mm256_xor_si256(c[1], rka[1]), _mm256_xor_si256(c[2], rkb)), 5);
    c[2] = rotr32_vec(_mm256_add_epi32(_mm256_xor_si256(c[2], rka[2]), _mm256_xor_si256(c[3], rkb)), 3);
    c[3] = c0_old;
}

#endif

void LeaVecState_block(LeaVecState *obj)
{
#ifdef LEA_VEC_ENABLED
    __m256i ca[4], cb[4];
    uint32_t *rk = obj->rk;
    leavec_load_ctr(ca, obj->ctr);
    leavec_load_ctr(cb, obj->ctr + LEA_NCOPIES / 2);
    for (int i = 0; i < LEA_NROUNDS; i++) {
        __m256i rkv[4];
        rkv[0] = _mm256_set1_epi32(rk[0]);
        rkv[1] = _mm256_set1_epi32(rk[1]);
        rkv[2] = _mm256_set1_epi32(rk[2]);
        rkv[3] = _mm256_set1_epi32(rk[3]);
        leavec_round(ca, rkv, rkv[3]);
        leavec_round(cb, rkv, rkv[3]);
        rk += LEA_RK_ALIGN;
    }
    leavec_store_out(obj->out, ca);
    leavec_store_out(obj->out + LEA_NCOPIES / 2, cb);
#else
    // AVX2 not supported
    (void) obj;
#endif
}

static inline void LeaState_inc_counter(LeaState *obj)
{
    uint64_t *ctr = (uint64_t *) obj->ctr;
    (*ctr)++;
}

/**
 * @brief Increase internal counters. There are 8 64-bit counters in the AVX2
 * version of the LEA based PRNG.
 * @relates LeaVecState
 */
static inline void LeaVecState_inc_counter(LeaVecState *obj)
{
    for (int i = 0; i < LEA_NCOPIES; i++) {
        obj->ctr[i] += LEA_NCOPIES;
    }
    // 32-bit counters overflow: increment the upper halves
    // We treat our counters as 64-bit, the upper half of the 128-bit block
    // is not changed (may be used as a thread ID if desired)
    if (obj->ctr[0] == 0) {
        for (int i = LEA_NCOPIES; i < 2 * LEA_NCOPIES; i++) {
            obj->ctr[i]++;
        }
    }
}


void LeaState_block_func(void *data)
{
    LeaState *obj = data;
    LeaState_block(obj);
    LeaState_inc_counter(obj);
    obj->intf.pos = 0;
}

void LeaVecState_block_func(void *data)
{
    LeaVecState *obj = data;
    LeaVecState_block(obj);
    LeaVecState_inc_counter(obj);
    obj->intf.pos = 0;
}


void LeaState_init(LeaState *obj, const uint32_t *key)
{
    lea128_fill_round_keys(obj->rk, key);
    for (int i = 0; i < 4; i++) {
        obj->ctr[i] = 0;
    }
    obj->intf.pos = 4;
    obj->intf.bufsize = 4;
    obj->intf.block_func = LeaState_block_func;
    obj->intf.out = obj->out;
}

void LeaVecState_init(LeaVecState *obj, const uint32_t *key)
{
    lea128_fill_round_keys(obj->rk, key);
    for (int i = 0; i < LEA_NCOPIES; i++) {
        obj->ctr[i] = i;
    }
    for (int i = LEA_NCOPIES; i < 4 * LEA_NCOPIES; i++) {
        obj->ctr[i] = 0;
    }
    obj->intf.pos = 4 * LEA_NCOPIES;
    obj->intf.bufsize = 4 * LEA_NCOPIES;
    obj->intf.block_func = LeaVecState_block_func;
    obj->intf.out = obj->out;
}


static inline uint64_t get_bits_raw(void *state)
{
    LeaInterface *obj = state;
    if (obj->pos >= obj->bufsize) {
        obj->block_func(obj);
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    uint32_t seeds[4];
    for (int i = 0; i < 2; i++) {
        uint64_t s = intf->get_seed64();
        seeds[2*i] = s & 0xFFFFFFF;
        seeds[2*i + 1] = s >> 32;
    }
    const char *ver = intf->get_param();
    if (!intf->strcmp(ver, "scalar") || !intf->strcmp(ver, "")) {
        intf->printf("LEA128-scalar\n");
        LeaState *obj = intf->malloc(sizeof(LeaState));
        LeaState_init(obj, seeds);
        return obj;
    } else if (!intf->strcmp(ver, "avx2")) {
#ifdef LEA_VEC_ENABLED
        intf->printf("LEA128-AVX2\n");
        LeaVecState *obj = intf->malloc(sizeof(LeaVecState));
        LeaVecState_init(obj, seeds);
        return obj;
#else
        intf->printf("AVX2 is not supported at this platform\n");
        return NULL;
#endif
    } else {
        intf->printf("Unknown version '%s' (scalar or avx2 are supported)", ver);
        return NULL;
    }
}


static int test_round_keys(const CallerAPI *intf, const uint32_t *rk, const uint32_t *rk_ref)
{
    int is_ok = 1;
    intf->printf("Testing round keys\n");
    intf->printf("%8s %8s\n", "rk", "rk(ref)");
    for (int i = 0; i < 4; i++) {
        intf->printf("%8X %8X\n", rk[i], rk_ref[i]);
        if (rk[i] != rk_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

/**
 * @brief An internal self-test for the scalar version of LEA128 with 128-bit key.
 * @details The test vectors from the original ISO were used.
 */
static int test_scalar(const CallerAPI *intf)
{
    static const uint32_t key[4] = {0x3c2d1e0f, 0x78695a4b, 0xb4a59687, 0xf0e1d2c3};    
    static const uint32_t rk23[4] = {0x0bf6adba, 0x5b72305a, 0xcb47c19f, 0xdf69029d};
    static const uint32_t out_ref[4] = {0x354ec89f, 0x18c6c628, 0xa7c73255, 0xfd8b6404};
    int is_ok = 1;
    LeaState *obj = intf->malloc(sizeof(LeaState));
    LeaState_init(obj, key);
    is_ok = is_ok & test_round_keys(intf, &obj->rk[(LEA_NROUNDS - 1) * LEA_RK_ALIGN], rk23);
    intf->printf("Output (ciphertext)\n");
    obj->ctr[0] = 0x13121110; obj->ctr[1] = 0x17161514;
    obj->ctr[2] = 0x1b1a1918; obj->ctr[3] = 0x1f1e1d1c;
    LeaState_block(obj);
    for (int i = 0; i < 4; i++) {
        intf->printf("%8X | %8X\n", obj->out[i], out_ref[i]);
        if (obj->out[i] != out_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}

/**
 * @brief An internal self-test for the vectorized (AVX2) version of LEA128
 * with 128-bit key.
 * @details It uses an extra test with several blocks and ECB mode to detect
 * possible errors in vectorization. Tests vectors are taken from:
 * - https://github.com/weidai11/cryptopp/blob/master/TestVectors/lea.txt
 *
 * The test vectors are:
 *
 *     Key:        54068DD2'68A46B55'CA03FCD4'F4C62B1C
 *     Plaintext:  D72E069A'7A307910'E5CB5C8C'3D98B19B
 *                 30A326BA'9479E20D'4A827D54'6991501A
 *                 98BAF02F'BC64F559'D49E0047'20B7FCC6
 *     Ciphertext: 6C83D52A'769B4146'F77EFB6F'64193D9A
 *                 B4763140'CB560574'792788D8'D051A6F8
 *                 42A3C6A7'31A9D88A'D0AAF959'F82309C3
 *
 * NOTE: these hexadecimal dump is for bytes, not for 32-bit words!
 */
static int test_vector(const CallerAPI *intf)
{
    static const uint32_t key[4] = {0x3c2d1e0f, 0x78695a4b, 0xb4a59687, 0xf0e1d2c3};    
    static const uint32_t rk23[4] = {0x0bf6adba, 0x5b72305a, 0xcb47c19f, 0xdf69029d};
    static const uint32_t out_ref[4] = {0x354ec89f, 0x18c6c628, 0xa7c73255, 0xfd8b6404};

    static const uint32_t key2[4] = {0xD28D0654, 0x556BA468, 0xD4FC03CA, 0x1C2BC6F4};    
    static const uint32_t in2[12] = {
        0x9A062ED7, 0x1079307A, 0x8C5CCBE5, 0x9BB1983D,
        0xBA26A330, 0x0DE27994, 0x547D824A, 0x1A509169,
        0x2FF0BA98, 0x59F564BC, 0x47009ED4, 0xC6FCB720};
    static const uint32_t out2[12] = {
        0x2AD5836C, 0x46419B76, 0x6FFB7EF7, 0x9A3D1964,
        0x403176B4, 0x740556CB, 0xD8882779, 0xF8A651D0,
        0xA7C6A342, 0x8AD8A931, 0x59F9AAD0, 0xC30923F8};
    int is_ok = 1;
    LeaVecState *obj = intf->malloc(sizeof(LeaVecState));
    LeaVecState_init(obj, key);
    is_ok = is_ok & test_round_keys(intf, &obj->rk[(LEA_NROUNDS - 1) * LEA_RK_ALIGN], rk23);
    intf->printf("Output (ciphertext)\n");
    for (int i = 0; i < LEA_NCOPIES; i++) {
        obj->ctr[i]                 = 0x13121110;
        obj->ctr[i + LEA_NCOPIES]   = 0x17161514;
        obj->ctr[i + 2*LEA_NCOPIES] = 0x1b1a1918;
        obj->ctr[i + 3*LEA_NCOPIES] = 0x1f1e1d1c;
    }
    LeaVecState_block(obj);
    for (int i = 0; i < 4 * LEA_NCOPIES; i++) {
        uint32_t u_ref = out_ref[i / LEA_NCOPIES];
        if (i % 4 == 0 && i > 0) {
            intf->printf("\n");
        }
        intf->printf("(%8X | %8X) ", obj->out[i], u_ref);
        if (obj->out[i] != u_ref) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    intf->printf("-------------------\n");
    // Non-repeating ciphertext
    LeaVecState_init(obj, key2);
    for (int i = 0; i < LEA_NCOPIES; i++) {
        int block_ind = i % 3;
        obj->ctr[i]                 = in2[0 + 4 * block_ind];
        obj->ctr[i + LEA_NCOPIES]   = in2[1 + 4 * block_ind];
        obj->ctr[i + 2*LEA_NCOPIES] = in2[2 + 4 * block_ind];
        obj->ctr[i + 3*LEA_NCOPIES] = in2[3 + 4 * block_ind];
    }
    LeaVecState_block(obj);
    for (int i = 0; i < LEA_NCOPIES; i++) {
        int block_ind = i % 3;
        intf->printf("COPY %2d CALC: ", i);
        for (int j = 0; j < 4; j++) {
            intf->printf("%8X ", obj->out[j * LEA_NCOPIES + i]);
        }
        intf->printf("\n");
        intf->printf("COPY %2d REF:  ", i);
        for (int j = 0; j < 4; j++) {
            intf->printf("%8X", out2[j + 4 * block_ind]);
            if (out2[j + 4 * block_ind] != obj->out[j * LEA_NCOPIES + i]) {
                intf->printf("<");
                is_ok = 0;
            } else {
                intf->printf(" ");
            }
        }
        intf->printf("\n");
    }
    intf->printf("\n");
    intf->free(obj);
    return is_ok;
}

/**
 * @brief Internal self-test.
 */
static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    is_ok = is_ok & test_scalar(intf);
    is_ok = is_ok & test_vector(intf);
    return is_ok;
}

MAKE_UINT32_PRNG("LEA128", run_self_test)
