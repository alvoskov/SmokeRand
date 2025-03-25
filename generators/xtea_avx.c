/**
 * @file xtea_avx.c
 * @brief An implementation of XTEA: a 64-bit block cipher with a 128-bit key.
 * @details XTEA is used as a "lightweight cryptography" for embedded system
 * but is rather slow (comparable to DES) on modern x86-64 processors. It is
 * suspectible to birthday paradox attack in CTR mode (fails the `birthday`
 * battery). Even in CBC mode it is prone to Sweet32 attack.
 *
 * References:
 *
 * - https://www.cix.co.uk/~klockstone/xtea.pdf
 * - https://www.cix.co.uk/~klockstone/teavect.htm
 * - https://tayloredge.com/reference/Mathematics/XTEA.pdf
 *
 * Results in CTR mode:
 *
 * - 4*2=8 rounds: fails `express`
 * - 5*2=10 rounds: passes `express`, passes `default`, fails `full` batteries.
 *   (`sumcollector` test)
 * - 6*2=12 rounds: passes `full` (tested only on `sumcollector`).
 *
 * @copyright Implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

PRNG_CMODULE_PROLOG


/**
 * @brief XTEA vectorized PRNG state. It contains 8 copies of XTEA and can work
 * either in CTR or CBC operation mode.
 * @details The next layout is used for both input (plaintext) and output
 * (ciphertext):
 *
 * \f[
 * \left[ x^{low}_0, \ldots, x^{low}_7, x^{high}_0,\ldots,x^{high}_7 \right]
 * \f]
 */
typedef struct {
    uint32_t in[16]; ///< Counters (plaintext).
    uint32_t out[16]; ///< Output (ciphertext).
    uint32_t key[4]; ///< 256-bit key.
    int pos; ///< Current position in the buffer (from 0 to 7).
    int is_cbc; ///< 0/1 - CTR/CBC operation mode.
} XteaVecState;


static inline __m256i mix(__m256i x, __m256i key)
{
    return _mm256_xor_si256(key, _mm256_add_epi32(x,
        _mm256_xor_si256(_mm256_slli_epi32(x, 4), _mm256_srli_epi32(x, 5))
    ));
}

/**
 * @brief XTEA encryption function (a vectorized version).
 * @relates XteaVecState
 */
void XteaVecState_block(XteaVecState *obj)
{
    static const uint32_t DELTA = 0x9e3779b9;
    uint32_t sum = 0;
    __m256i y = _mm256_loadu_si256((__m256i *) obj->in);
    __m256i z = _mm256_loadu_si256((__m256i *) (obj->in + 8));
    // CBC mode: XOR input (counter) with the previous input
    if (obj->is_cbc) {
        __m256i out0 = _mm256_loadu_si256((__m256i *) obj->out);
        __m256i out1 = _mm256_loadu_si256((__m256i *) (obj->out + 8));
        y = _mm256_xor_si256(y, out0);
        z = _mm256_xor_si256(z, out1);
    }
    for (int i = 0; i < 32; i++) {
        __m256i keyA = _mm256_set1_epi32(sum + obj->key[sum & 3]);
        y = _mm256_add_epi32(y, mix(z, keyA));
        sum += DELTA;
        __m256i keyB = _mm256_set1_epi32(sum + obj->key[(sum >> 11) & 3]);
        z = _mm256_add_epi32(z, mix(y, keyB));
    }
    _mm256_storeu_si256((__m256i *) obj->out, y);
    _mm256_storeu_si256((__m256i *) (obj->out + 8), z);
}

/**
 * @brief Initializes an example of XTEA vectorized PRNG.
 * @param obj Pointer to the generator to be initialized.
 * @param key 256-bit key.
 * @relates XteaVecState
 */
void XteaVecState_init(XteaVecState *obj, const uint32_t *key)
{
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key[i];
    }
    for (int i = 0; i < 8; i++) {
        obj->in[i] = i;
    }
    for (int i = 8; i < 16; i++) {
        obj->in[i] = 0;
    }
    for (int i = 0; i < 16; i++) {
        obj->out[i] = 0; // Needed for CBC mode
    }
    obj->pos = 8;
    obj->is_cbc = 0;
}

/**
 * @brief Increase internal counters. There are 8 64-bit counters in the AVX2
 * version of the XTEA based PRNG.
 * @relates XteaVecState
 */
static inline void XteaVecState_inc_ctr(XteaVecState *obj)
{
    for (int i = 0; i < 8; i++) {
        obj->in[i] += 8;
    }
    // 32-bit counters overflow: increment the upper halves
    if (obj->in[0] == 0) {
        for (int i = 8; i < 16; i++) {
            obj->in[i]++;
        }
    }
}


static inline uint64_t get_bits_raw(void *state)
{
    XteaVecState *obj = state;
    if (obj->pos >= 8) {
        XteaVecState_block(obj);
        XteaVecState_inc_ctr(obj);
        obj->pos = 0;
    }
    uint64_t val = obj->out[obj->pos];
    val |= ((uint64_t) obj->out[obj->pos + 8]) << 32;
    obj->pos++;
    return val;
}


static void *create(const CallerAPI *intf)
{
    XteaVecState *obj = intf->malloc(sizeof(XteaVecState));
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    uint32_t key[4];
    key[0] = (uint32_t) s0; key[1] = s0 >> 32;
    key[2] = (uint32_t) s1; key[3] = s1 >> 32;
    XteaVecState_init(obj, key);
    const char *mode_name = intf->get_param();
    if (!intf->strcmp(mode_name, "ctr") || !intf->strcmp(mode_name, "")) {
        obj->is_cbc = 0;
        intf->printf("Operation mode: ctr\n");
    } else if (!intf->strcmp(mode_name, "cbc")) {
        obj->is_cbc = 1;
        intf->printf("Operation mode: cbc\n");
    } else {
        intf->printf("Unknown operation mode '%s' (ctr or cbc are supported)",
            mode_name);
        intf->free(obj);
        return NULL;
    }
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    XteaVecState *obj = intf->malloc(sizeof(XteaVecState));
    const uint64_t u_ref = 0x0A202283D26428AF;
    const uint32_t key[4] = {0x27F917B1, 0xC1DA8993, 0x60E2ACAA, 0xA6EB923D};
    uint64_t u;
    XteaVecState_init(obj, key);
    for (int i = 0; i < 8; i++) {
        obj->in[i]     = 0xAF20A390;
        obj->in[i + 8] = 0x547571AA;
    }
    for (int i = 0; i < 8; i++) {
        u = get_bits_raw(obj);
    }
    intf->printf("Results: out = %llX; ref = %llX\n",
        (unsigned long long) u,
        (unsigned long long) u_ref);
    intf->free(obj);
    return u == u_ref;    
}

MAKE_UINT64_PRNG("XTEA_AVX", run_self_test)
