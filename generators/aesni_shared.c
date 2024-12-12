/**
 * @file aesni_shared.c
 * @brief An implementation of CSPRNG based on AES128 in the counter mode.
 * @details It uses AESNI instructions for x86-64 architecture: hardware AES
 * implementation is much simpler and faster than software implementation.
 *
 * Test vectors are taken from [1], see Chapter F.5.1.
 *
 * References:
 *
 * 1. Morris Dworkin. NIST Special Publication 800-38A. Recommendation for
 *    Block Cipher Modes of Operation. Methods and Techniques. 2001 edition.
 *    https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf
 * 2. https://gist.github.com/acapola/d5b940da024080dfaf5f
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


/**
 * @brief AES-128 counter mode state designed as pseudorandom number
 * generator.
 */
typedef struct {
    __m128i key_schedule[11]; ///< The expanded key.
    uint64_t ctr[2]; ///< 128-bit counter.
    uint64_t out[2]; ///< 128-bit output buffer.
    int pos; ///< Current position in the output buffer.
} AES128State;

/**
 * @brief AES128 key in the format friendly to construction from 64-bit seeds.
 */
typedef union {
    uint64_t w64[2];
    uint8_t b[16];
} AES128Key;

static __m128i AES128_expand_key(__m128i key, unsigned int rc)
{
    __m128i keygened = _mm_aeskeygenassist_si128(key, rc);
	keygened = _mm_shuffle_epi32(keygened, _MM_SHUFFLE(3,3,3,3));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	return _mm_xor_si128(key, keygened);
}

/**
 * @brief Initialize AES128 based PRNG state, i.e. fill key schedule
 * and counters.
 */
void AES128State_init(AES128State *obj, const AES128Key *enc_key)
{
    __m128i *ks = obj->key_schedule;
    ks[0]  = _mm_loadu_si128((const __m128i*)(enc_key->b));
    ks[1]  = AES128_expand_key(ks[0], 0x01);
    ks[2]  = AES128_expand_key(ks[1], 0x02);
    ks[3]  = AES128_expand_key(ks[2], 0x04);
    ks[4]  = AES128_expand_key(ks[3], 0x08);
    ks[5]  = AES128_expand_key(ks[4], 0x10);
    ks[6]  = AES128_expand_key(ks[5], 0x20);
    ks[7]  = AES128_expand_key(ks[6], 0x40);
    ks[8]  = AES128_expand_key(ks[7], 0x80);
    ks[9]  = AES128_expand_key(ks[8], 0x1B);
    ks[10] = AES128_expand_key(ks[9], 0x36);
    obj->ctr[0] = obj->ctr[1] = 0;
    obj->pos = 2;
}

/**
 * @brief Encode (encrypt) 128-bit block.
 * @param[out] outout Output data(128-bit block).
 * @param[in] input Input data (128-bit block).
 */
void AES128State_encode(AES128State *obj, uint8_t *output, uint8_t *input)
{
    __m128i m = _mm_loadu_si128((__m128i *) input);
    __m128i *ks = obj->key_schedule;
    m = _mm_xor_si128(m, ks[0]);
    m = _mm_aesenc_si128(m, ks[1]);
    m = _mm_aesenc_si128(m, ks[2]);
    m = _mm_aesenc_si128(m, ks[3]);
    m = _mm_aesenc_si128(m, ks[4]);
    m = _mm_aesenc_si128(m, ks[5]);
    m = _mm_aesenc_si128(m, ks[6]);
    m = _mm_aesenc_si128(m, ks[7]);
    m = _mm_aesenc_si128(m, ks[8]);
    m = _mm_aesenc_si128(m, ks[9]);
    m = _mm_aesenclast_si128(m, ks[10]);
    _mm_storeu_si128((__m128i *) output, m);
}

/**
 * @brief Returns 64-bit unsigned integer from the 128-bit output buffer.
 */
static inline uint64_t get_bits_raw(void *state)
{
    AES128State *obj = state;
    if (obj->pos == 2) {
        AES128State_encode(obj, (uint8_t *) obj->out, (uint8_t *) obj->ctr);
        if (++obj->ctr[0] == 0) {
            obj->ctr[1]++;
        }
        obj->pos = 0;
    }
    return obj->out[obj->pos++];    
}

/**
 * @brief Initializes AES128 based PRNG. Two random 64-bit seeds are used
 * as a key.
 */
static void *create(const CallerAPI *intf)
{
    AES128State *obj = intf->malloc(sizeof(AES128State));
    AES128Key key;
    key.w64[0] = intf->get_seed64();
    key.w64[1] = intf->get_seed64();
    AES128State_init(obj, &key);
    return (void *) obj;
}

/**
 * @brief An internal self-test based on NIST Special Publication 800-38A
 * by Morris Dworkin.
 */
int run_self_test(const CallerAPI *intf)
{
    uint8_t input[] = {
        0xf0, 0xf1, 0xf2, 0xf3,  0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb,  0xfc, 0xfd, 0xfe, 0xff
    };
    uint8_t output_ref[] = {
        0xec, 0x8c, 0xdf, 0x73,  0x98, 0x60, 0x7c, 0xb0,
        0xf2, 0xd2, 0x16, 0x75,  0xea, 0x9e, 0xa1, 0xe4
    };
    AES128Key key = {
        .b = {
            0x2b, 0x7e, 0x15, 0x16,  0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88,  0x09, 0xcf, 0x4f, 0x3c}
    };
    uint8_t output_comp[16];
    int is_ok = 1;
    AES128State *obj = intf->malloc(sizeof(AES128State));
    AES128State_init(obj, &key);
    AES128State_encode(obj, output_comp, input);
    intf->printf("Output:      ");
    for (int i = 0; i < 16; i++) {
        intf->printf("%.2X ", output_comp[i]);
        if (output_comp[i] != output_ref[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\nReference:   ");
    for (int i = 0; i < 16; i++) {
        intf->printf("%.2X ", output_ref[i]);
    }
    intf->printf("\n");
    intf->free(obj);
    return is_ok;
}

MAKE_UINT64_PRNG("AES128", run_self_test)
