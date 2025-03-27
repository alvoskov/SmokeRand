/**
 * @file xtea.c
 * @brief An implementation of PRNG based on XXTEA with 128-bit blocks.
 * @details XXTEA is used as a "lightweight cryptography" for embedded systems.
 *
 * References:
 * 1. https://www.movable-type.co.uk/scripts/xxtea.pdf
 * 2. https://ia.cr/2010/254
 * 3. https://github.com/an0maly/Crypt-XXTEA/blob/master/reference/test-vector.t
 *
 * Testing:
 * - 2 rounds: fails `express` battery.
 * - 3 rounds: fails `full` battery, `sumcollector` test.
 * - 4 rounds: passes `full` battery.
 *
 * @copyright Implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#ifdef  __AVX2__
#define XXTEA_VEC_ENABLED
#endif

#ifdef XXTEA_VEC_ENABLED
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

enum {
    XXTEA_DELTA = 0x9e3779b9,
    XXTEA_NROUNDS = 19
};

/**
 * @brief XXTEA PRNG state.
 */
typedef struct {
    BufGen32Interface intf;
    uint32_t ctr[4];
    uint32_t key[4];
    uint32_t out[4];
    int pos;
} XxteaState;

/**
 * @brief Number of generator copies inside the state.
 */
#define XXTEA_NCOPIES 8

/**
 * @brief XXTEA PRNG state.
 */
typedef struct {
    BufGen32Interface intf;
    uint32_t ctr[4 * XXTEA_NCOPIES];
    uint32_t key[4];
    uint32_t out[4 * XXTEA_NCOPIES];
} XxteaVecState;

///////////////////////////////////////////
///// XxteaState class implementation /////
///////////////////////////////////////////

/**
 * @brief XXTEA mixing function.
 */
#define MX(p) (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (obj->key[((p)&3)^e] ^ z)))

static inline uint32_t mix(uint32_t y, uint32_t z, uint32_t sum, uint32_t rk)
{
    return (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (rk ^ z)));
}

/**
 * @brief XXTEA encryption subroutine.
 */
void XxteaState_block(XxteaState *obj)
{
    uint32_t y = obj->ctr[0], z = obj->ctr[3], sum = 0;
    for (int i = 0; i < 4; i++) {
        obj->out[i] = obj->ctr[i];
    }
    for (int i = 0; i < XXTEA_NROUNDS; i++) {
        sum += XXTEA_DELTA;
        unsigned int e = (sum >> 2) & 3;
        y = obj->out[1]; 
        z = obj->out[0] += mix(y, z, sum, obj->key[0 ^ e]);
        y = obj->out[2]; 
        z = obj->out[1] += mix(y, z, sum, obj->key[1 ^ e]);
        y = obj->out[3]; 
        z = obj->out[2] += mix(y, z, sum, obj->key[2 ^ e]);
        y = obj->out[0];
        z = obj->out[3] += mix(y, z, sum, obj->key[3 ^ e]);
    }
}

/**
 * @brief Increase the internal 64-bit counter.
 */
static inline void XxteaState_inc_counter(XxteaState *obj)
{
    uint64_t *ctr = (uint64_t *) obj->ctr;
    (*ctr)++;
}

/**
 * @brief Generates a new block of pseudorandom numbers and updates
 * internal counters.
 * @relates LeaState.
 */
void XxteaState_iter_func(void *data)
{
    XxteaState *obj = data;
    XxteaState_block(obj);
    XxteaState_inc_counter(obj);
    obj->intf.pos = 0;
}



//////////////////////////////////////////////
///// XxteaVecState class implementation /////
//////////////////////////////////////////////


/**
 * @brief XXTEA mixing function.
 */
static inline __m256i mixv(__m256i y, __m256i z, __m256i sum, __m256i rk)
{
    return _mm256_xor_si256(
        _mm256_add_epi32(
            _mm256_xor_si256(_mm256_srli_epi32(z, 5), _mm256_slli_epi32(y, 2)),
            _mm256_xor_si256(_mm256_srli_epi32(y, 3), _mm256_slli_epi32(z, 4))
        ),
        _mm256_add_epi32(_mm256_xor_si256(sum, y), _mm256_xor_si256(rk, z))
    );
}

/**
 * @brief XXTEA encryption subroutine.
 */
void XxteaVecState_block(XxteaVecState *obj)
{
    uint32_t sum = 0;
    __m256i out[4], y, z;
    out[0] = _mm256_loadu_si256((__m256i *) obj->ctr);
    out[1] = _mm256_loadu_si256((__m256i *) (obj->ctr + XXTEA_NCOPIES));
    out[2] = _mm256_loadu_si256((__m256i *) (obj->ctr + 2*XXTEA_NCOPIES));
    out[3] = _mm256_loadu_si256((__m256i *) (obj->ctr + 3*XXTEA_NCOPIES));
    y = out[0], z = out[3];
    for (int i = 0; i < XXTEA_NROUNDS; i++) {
        sum += XXTEA_DELTA;
        unsigned int e = (sum >> 2) & 3;
        __m256i sumv = _mm256_set1_epi32(sum);
        __m256i rk[4];
        rk[0] = _mm256_set1_epi32(obj->key[0 ^ e]);
        rk[1] = _mm256_set1_epi32(obj->key[1 ^ e]);
        rk[2] = _mm256_set1_epi32(obj->key[2 ^ e]);
        rk[3] = _mm256_set1_epi32(obj->key[3 ^ e]);
        y = out[1]; 
        z = _mm256_add_epi32(out[0], mixv(y, z, sumv, rk[0])); out[0] = z;
        y = out[2]; 
        z = _mm256_add_epi32(out[1], mixv(y, z, sumv, rk[1])); out[1] = z;
        y = out[3]; 
        z = _mm256_add_epi32(out[2], mixv(y, z, sumv, rk[2])); out[2] = z;
        y = out[0];
        z = _mm256_add_epi32(out[3], mixv(y, z, sumv, rk[3])); out[3] = z;
    }
    _mm256_storeu_si256((__m256i *) obj->out,                     out[0]);
    _mm256_storeu_si256((__m256i *) (obj->out + XXTEA_NCOPIES),   out[1]);
    _mm256_storeu_si256((__m256i *) (obj->out + 2*XXTEA_NCOPIES), out[2]);
    _mm256_storeu_si256((__m256i *) (obj->out + 3*XXTEA_NCOPIES), out[3]);
}


/**
 * @brief Increase the internal 64-bit counter.
 */
static inline void XxteaVecState_inc_counter(XxteaVecState *obj)
{
    for (int i = 0; i < XXTEA_NCOPIES; i++) {
        obj->ctr[i] += XXTEA_NCOPIES;
    }
    // 32-bit counters overflow: increment the upper halves
    if (obj->ctr[0] == 0) {
        for (int i = XXTEA_NCOPIES; i < 2 * XXTEA_NCOPIES; i++) {
            obj->ctr[i]++;
        }
    }
}

/**
 * @brief Generates a new block of pseudorandom numbers and updates
 * internal counters.
 * @relates LeaState.
 */
void XxteaVecState_iter_func(void *data)
{
    XxteaVecState *obj = data;
    XxteaVecState_block(obj);
    XxteaVecState_inc_counter(obj);
    obj->intf.pos = 0;
}

/**
 * @brief Initializes an example of XXTEA vectorized PRNG.
 * @param obj Pointer to the generator to be initialized.
 * @param key 256-bit key.
 * @relates XteaVecState
 */
void XxteaVecState_init(XxteaVecState *obj, const uint32_t *key)
{
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key[i];
    }
    for (int i = 0; i < XXTEA_NCOPIES; i++) {
        obj->ctr[i] = i;
    }
    for (int i = XXTEA_NCOPIES; i < 4 * XXTEA_NCOPIES; i++) {
        obj->ctr[i] = 0;
    }
    obj->intf.iter_func = XxteaVecState_iter_func;
    obj->intf.pos = 4 * XXTEA_NCOPIES;
    obj->intf.bufsize = 4 * XXTEA_NCOPIES;
    obj->intf.out = obj->out;
}

//////////////////////
///// Interfaces /////
//////////////////////

BUFGEN32_DEFINE_GET_BITS_RAW


static void *create(const CallerAPI *intf)
{
    uint32_t key[4];
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    key[0] = (uint32_t) s0; key[1] = s0 >> 32;
    key[2] = (uint32_t) s1; key[3] = s1 >> 32;
    const char *ver = intf->get_param();
    if (!intf->strcmp(ver, "scalar") || !intf->strcmp(ver, "")) {
        XxteaState *obj = intf->malloc(sizeof(XxteaState));
        for (int i = 0; i < 4; i++) {
            obj->key[i] = key[i];
        }
        obj->intf.iter_func = XxteaState_iter_func;
        obj->intf.pos = 4;
        obj->intf.bufsize = 4;
        obj->intf.out = obj->out;
        return obj;
    } else if (!intf->strcmp(ver, "avx2")) {
        XxteaVecState *obj = intf->malloc(sizeof(XxteaVecState));
        XxteaVecState_init(obj, key);
        return obj;
    } else {
        intf->printf("Unknown version '%s' (scalar or avx2 are supported)", ver);
        return NULL;
    }
}

///////////////////////////////
///// Internal self-tests /////
///////////////////////////////

static int cmp_vec4(const CallerAPI *intf, const uint32_t *out, const uint32_t *ref)
{
    int is_ok = 1;
    intf->printf("\nOUT: ");
    for (int i = 0; i < 4; i++) {
        intf->printf("0x%.8X ", out[i]);
    }
    intf->printf("\nREF: ");
    for (int i = 0; i < 4; i++) {
        intf->printf("0x%.8X ", ref[i]);
        if (ref[i] != out[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    return is_ok;
}

static int scalar_test(const CallerAPI *intf)
{
    static const uint32_t
        key[4]  = {0xb979379e, 0xe973979b, 0x9e3779b9, 0x5651696b},
        in[4]   = {0x08040201, 0x80402010, 0xf8fcfeff, 0x80c0e0f0},
        ref[4]  = {0xfd15b801, 0xd194482e, 0x43da5535, 0x8a869d4c},
        key2[4] = {0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa},
        ref2[4] = {0x069fa1c0, 0x39d6b0eb, 0xf727aa25, 0xd0b2c64c};

    XxteaState *obj = intf->malloc(sizeof(XxteaState));
    int is_ok = 1;
    // Test 1
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key[i];
        obj->ctr[i] = in[i];
    }
    XxteaState_block(obj);
    is_ok = is_ok & cmp_vec4(intf, obj->out, ref);
    // Test 2
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key2[i];
    }
    XxteaState_block(obj);
    is_ok = is_ok & cmp_vec4(intf, obj->out, ref2);
    intf->free(obj);
    return is_ok;
}

/**
 * @details Test vectors are taken from:
 * https://github.com/an0maly/Crypt-XXTEA/blob/master/reference/test-vector.t
 */
static int vector_test(const CallerAPI *intf)
{
    static const uint32_t
        key[4]  = {0xb979379e, 0xe973979b, 0x9e3779b9, 0x5651696b},
        in[4]   = {0x08040201, 0x80402010, 0xf8fcfeff, 0x80c0e0f0},
        ref[4]  = {0xfd15b801, 0xd194482e, 0x43da5535, 0x8a869d4c};
    int is_ok = 1;
    XxteaVecState *obj = intf->malloc(sizeof(XxteaVecState));
    XxteaVecState_init(obj, key);
    // Test 1
    for (int i = 0; i < XXTEA_NCOPIES; i++) {
        obj->ctr[i]                   = in[0];
        obj->ctr[i + XXTEA_NCOPIES]   = in[1];
        obj->ctr[i + 2*XXTEA_NCOPIES] = in[2];
        obj->ctr[i + 3*XXTEA_NCOPIES] = in[3];
    }
    XxteaVecState_block(obj);
    for (int i = 0; i < XXTEA_NCOPIES; i++) {
        intf->printf("COPY %2d: ", i);
        for (int j = 0; j < 4; j++) {
            uint32_t u = obj->out[i + XXTEA_NCOPIES*j];
            intf->printf("0x%.8X ", u);
            if (u != ref[j]) {
                is_ok = 0;
            }
        }
        intf->printf("\n");
    }
    intf->printf("REF:     ");
    for (int i = 0; i < 4; i++) {
        intf->printf("0x%.8X ", ref[i]);
    }
    intf->printf("\n");
    intf->free(obj);
    return is_ok;
}

/**
 * @details Test vectors are taken from:
 * https://github.com/an0maly/Crypt-XXTEA/blob/master/reference/test-vector.t
 */
static int run_self_test(const CallerAPI *intf)
{
    int is_ok = scalar_test(intf);
    is_ok = is_ok & vector_test(intf);
    return is_ok;
}

MAKE_UINT32_PRNG("XXTEA", run_self_test)
