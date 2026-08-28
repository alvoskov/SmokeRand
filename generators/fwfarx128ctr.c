/**
 * @file fwfarx128ctr.c
 * @brief fwfarx128ctr is a fast counter-based PRNG for 32-bit processors,
 * 128-bit seed/key and with 128-bit output block.
 * @details Its designed is based on an experimental ARX cipher suggested by
 * L. Hars and G. Petruska. The next modifications were made by A.L. Voskov
 * during the fwfarx128 design, they are intended to turn it into a fast
 * non-cryptographic 32-bit Counter-Based PRNG (CBPRNG):
 *
 * 1. Two different rotatations instead of one, dynamic indexing inside
 *    the key schedule was excluded.
 * 2. The key schedule is based on just scrambling the key with a combination
 *    of xorrot LFSR and a non-linear output function.
 * 3. Number of rounds were reduced, also fwfarx128ctr round corresponds to
 *    TWO rounds of the original cipher.
 *
 * Notes about rotations tuning:
 *
 * 1. They were manually tuned by means of hamming_distr test from default and
 *    full batteries of SmokeRand.
 * 2. 2 rounds: hamming_distr from full is ok (even without key addition)
 * 3. ctr[0] increment passes the entire "full" SmokeRand and >= 1 TiB
 *    in PractRand 0.96.
 *
 * WARNING! NOT FOR CRYPTOGRAPHY! Use only as a general purpose CBPRNG!
 *
 * References:
 *
 * 1. Hars L., Petruska G. Pseudorandom recursions II. // J Embedded Systems.
 *    2012, 1 (2012). https://doi.org/10.1186/1687-3963-2012-1
 * 2. Hars L., Petruska G. Pseudorandom Recursions: Small and Fast Pseudorandom
 *    Number Generators for Embedded Applications // J Embedded Systems 2007,
 *    098417 (2007). https://doi.org/10.1155/2007/98417
 * 3. Hars L. Hardware Bit-Mixers. Cryptology {ePrint} Archive, Paper 2017/084.
 *    https://eprint.iacr.org/2017/084
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#ifdef __AVX2__
#include "smokerand/x86exts.h"
#endif

PRNG_CMODULE_PROLOG


/////////////////////////////////
///// Some helper functions /////
/////////////////////////////////


void fwfarx128_mix_key(uint32_t *mixed_key, const uint32_t *key)
{
    uint32_t x = key[0], y = key[1], z = key[2], w = key[3];
    for (int i = 0; i < 64 + 4; i++) {
        const uint32_t x0 = x, w0 = w;
        x = x0 ^ y;
        y = z;
        z = x0 ^ w0;
        w = (x0 << 9) ^ z ^ rotl32(w0, 4) ^ rotl32(w0, 17);
        if (i >= 64) {
            uint32_t out = rotl32(69069U * x0, 5);
            out += (out * out | 0x4005);
            mixed_key[i - 64] = out;
        }
    }
}

////////////////////////////////////////////////////
///// Scalar (portable) version implementation /////
////////////////////////////////////////////////////

typedef struct {
    uint32_t key[4];
    uint32_t ctr[4];
    uint32_t out[4];
    int pos;
} Fwfarx128CtrState;


static inline uint64_t get_bits_scalar_raw(Fwfarx128CtrState *obj)
{
    const int sh1 = 19, sh2 = 5;
    if (obj->pos == 4) {
        uint32_t x = obj->ctr[0], y = obj->ctr[1];
        uint32_t z = obj->ctr[2], w = obj->ctr[3];
        for (int i = 0; i < 3; i++) {
            x += rotl32(y ^ z ^ w, sh1) + obj->key[0];
            y += rotl32(z ^ w ^ x, sh2) + obj->key[1];
            z += rotl32(w ^ x ^ y, sh1) + obj->key[2];
            w += rotl32(x ^ y ^ z, sh2) + obj->key[3];

            x += rotl32(y ^ z ^ w, sh2) + obj->key[0];
            y += rotl32(z ^ w ^ x, sh1) + obj->key[2];
            z += rotl32(w ^ x ^ y, sh2) + obj->key[1];
            w += rotl32(x ^ y ^ z, sh1) + obj->key[3];
        }
        obj->out[0] = x; obj->out[1] = y;
        obj->out[2] = z; obj->out[3] = w;
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


void Fwfarx128CtrState_init(Fwfarx128CtrState *obj, const uint32_t *key)
{
    // Key mixer
    fwfarx128_mix_key(obj->key, key);
    // Reset counter
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    obj->ctr[2] = 0; obj->ctr[3] = 0;
    obj->pos = 4;
}



static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    uint32_t k[4];
    Fwfarx128CtrState *obj = intf->malloc(sizeof(Fwfarx128CtrState));
    seeds_to_array_u32(intf, k, 4);
    Fwfarx128CtrState_init(obj, k);
    return obj;
}


MAKE_GET_BITS_WRAPPERS(scalar)


////////////////////////////////////////////////
///// AVX2 (vector) version implementation /////
////////////////////////////////////////////////

#define XORMIX128CTR_NCOPIES 8

typedef union {
    uint32_t u32[XORMIX128CTR_NCOPIES];
} Fwfarx128CtrElement;


typedef struct {
    uint32_t key[4];
    Fwfarx128CtrElement ctr[4]; ///< Counter
    Fwfarx128CtrElement out[4]; ///< Output state
    size_t pos;
} Fwfarx128CtrVecState;


#ifdef __AVX2__


static inline __m256i mm256_rotl_epi32_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi32(in, r), _mm256_srli_epi32(in, 32 - r));
}


static inline void fwfarx128ctr_vec_round(__m256i *s, __m256i *k)
{
    const int sh1 = 19, sh2 = 5;
    // --- Half-round 1 ---
    {
        // s[0] += rotl64(s[1] ^ s[2] ^ s[3], sh1) + k[0];
        __m256i s23 = _mm256_xor_si256(s[2], s[3]);
        s[0] = _mm256_add_epi32(s[0], mm256_rotl_epi32_def(_mm256_xor_si256(s[1], s23), sh1));
        s[0] = _mm256_add_epi32(s[0], k[0]);
        // s[1] += rotl64(s[2] ^ s[3] ^ s[0], sh2) + k[1];
        s[1] = _mm256_add_epi32(s[1], mm256_rotl_epi32_def(_mm256_xor_si256(s23, s[0]), sh2));
        s[1] = _mm256_add_epi32(s[1], k[1]);
        // s[2] += rotl64(s[3] ^ s[0] ^ s[1], sh1) + k[2];
        __m256i s01 = _mm256_xor_si256(s[0], s[1]);
        s[2] = _mm256_add_epi32(s[2], mm256_rotl_epi32_def(_mm256_xor_si256(s[3], s01), sh1));
        s[2] = _mm256_add_epi32(s[2], k[2]);
        // s[3] += rotl64(s[0] ^ s[1] ^ s[2], sh2) + k[3];
        s[3] = _mm256_add_epi32(s[3], mm256_rotl_epi32_def(_mm256_xor_si256(s01, s[2]), sh2));
        s[3] = _mm256_add_epi32(s[3], k[3]);
    }
    // --- Half-round 2 ---
    {
        __m256i s23 = _mm256_xor_si256(s[2], s[3]);
        s[0] = _mm256_add_epi32(s[0], mm256_rotl_epi32_def(_mm256_xor_si256(s[1], s23), sh2));
        s[0] = _mm256_add_epi32(s[0], k[0]);
        s[1] = _mm256_add_epi32(s[1], mm256_rotl_epi32_def(_mm256_xor_si256(s23, s[0]), sh1));
        s[1] = _mm256_add_epi32(s[1], k[2]);
        __m256i s01 = _mm256_xor_si256(s[0], s[1]);
        s[2] = _mm256_add_epi32(s[2], mm256_rotl_epi32_def(_mm256_xor_si256(s[3], s01), sh2));
        s[2] = _mm256_add_epi32(s[2], k[1]);
        s[3] = _mm256_add_epi32(s[3], mm256_rotl_epi32_def(_mm256_xor_si256(s01, s[2]), sh1));
        s[3] = _mm256_add_epi32(s[3], k[3]);
    }
}

static inline void Fwfarx128CtrVecState_block(Fwfarx128CtrVecState *obj)
{
    __m256i out[4], key[4];
    for (size_t i = 0; i < 4; i++) {
        out[i] = _mm256_loadu_si256((__m256i *) (void *) &obj->ctr[i].u32[0]);
        key[i] = _mm256_set1_epi32((int) obj->key[i]);
    }

    for (int i = 0; i < 3; i++) {
        fwfarx128ctr_vec_round(out, key);
    }

    for (size_t i = 0; i < 4; i++) {
        _mm256_storeu_si256((__m256i *) (void *) &obj->out[i].u32[0], out[i]);
    }
}

void Fwfarx128CtrVecState_init(Fwfarx128CtrVecState *obj, const uint32_t *key)
{
    // Key mixer    
    fwfarx128_mix_key(obj->key, key);
    // Reset counter
    for (size_t i = 0; i < XORMIX128CTR_NCOPIES; i++) {
        obj->ctr[0].u32[i] = (uint32_t) i; obj->ctr[1].u32[i] = 0;
        obj->ctr[2].u32[i] = 0;            obj->ctr[3].u32[i] = 0;
    }
    Fwfarx128CtrVecState_block(obj);
    obj->pos = 0;
}


static inline uint64_t get_bits_vector_raw(Fwfarx128CtrVecState *obj)
{
    const size_t i = (obj->pos & 0x3), j = (obj->pos >> 2);
    const uint32_t x = obj->out[i].u32[j];
    obj->pos++;
    if (obj->pos == 4 * XORMIX128CTR_NCOPIES) {
        for (size_t k = 0; k < XORMIX128CTR_NCOPIES; k++) {
            uint64_t ctr = (obj->ctr[0].u32[k]) | ((uint64_t) obj->ctr[1].u32[k] << 32);
            ctr += XORMIX128CTR_NCOPIES;
            obj->ctr[0].u32[k] = (uint32_t) ctr;
            obj->ctr[1].u32[k] = (uint32_t) (ctr >> 32);
        }
        Fwfarx128CtrVecState_block(obj);
        obj->pos = 0;
    }
    return x;
}



static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    uint32_t k[4];
    Fwfarx128CtrVecState *obj = intf->malloc(sizeof(Fwfarx128CtrVecState));
    seeds_to_array_u32(intf, k, 4);
    Fwfarx128CtrVecState_init(obj, k);
    return obj;
}

MAKE_GET_BITS_WRAPPERS(vector)


#endif

//////////////////////
///// Interfaces /////
//////////////////////

static void *create(const CallerAPI *intf)
{
    intf->printf("'%s' not implemented\n", intf->get_param());
    return NULL;
}

static const GeneratorParamVariant gen_list[] = {
    {"",     "fwfarx128ctr:c99",  32, create_scalar, get_bits_scalar, get_sum_scalar},
    {"c99",  "fwfarx128ctr:c99",  32, create_scalar, get_bits_scalar, get_sum_scalar},
#ifdef __AVX2__
    {"avx2", "fwfarx128ctr:avx2", 32, create_vector, get_bits_vector, get_sum_vector},
#endif
    GENERATOR_PARAM_VARIANT_EMPTY
};

/**
 * @brief An internal self-test that contains two subtests:
 *
 * 1. Compares the scalar version output with reference values.
 * 2. Compares outputs of the vector version with the output of
 *    the scalar version.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t k[4] = {
        0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344
    };
    static const uint32_t u_ref[16] = {
        0xD17C76C5, 0xBBEAF315, 0xDC996839, 0xD824DE49,
        0xC3BEE8A0, 0xE742D872, 0xAC0146A3, 0x2ED61447,
        0x21494D03, 0x029BA654, 0x27A81867, 0xCC69474B,
        0x5FB0D676, 0x9C5C2EB3, 0x728063D4, 0xE160B7E5
    };
    int is_ok = 1;
    Fwfarx128CtrState *obj = intf->malloc(sizeof(Fwfarx128CtrState));
    Fwfarx128CtrState_init(obj, k);
    for (int i = 0; i < 16; i++) {
        const uint32_t u = (uint32_t) get_bits_scalar_raw(obj);
        intf->printf("0x%8.8lX 0x%8.8lX\n",
            (unsigned long) u,
            (unsigned long) u_ref[i]
        );
        if (u_ref[i] != u) {
            is_ok = 0;
        }
    }

#ifdef __AVX2__
    Fwfarx128CtrVecState *obj_vec = intf->malloc(sizeof(Fwfarx128CtrVecState));
    Fwfarx128CtrVecState_init(obj_vec, k);
    for (int i = 0; i < 16; i++) {
        (void) get_bits_vector_raw(obj_vec);
    }
    for (int i = 0; i < 32768; i++) {
        if (get_bits_vector_raw(obj_vec) != get_bits_scalar_raw(obj)) {
            intf->printf("%d\n ", i);
            is_ok = 0;
            break;
        }
    }
    intf->free(obj_vec);
#endif
    intf->free(obj);
    return is_ok;    
}


static const char description[] =
"fwfarx128ctr is the counter-based PRNG with 128-bit block.\n"
"The next param values are supported:\n"
"  c99  - portable version (default version)\n"
"  avx2 - AVX2 vectorized version\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}

