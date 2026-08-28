/**
 * @file fwfarx256ctr.c
 * @brief fwfarx256ctr is a fast counter-based PRNG for 64-bit processors,
 * 256-bit seed/key and with 256-bit output block.
 * @details Its designed is based on an experimental ARX cipher suggested by
 * L. Hars and G. Petruska. The next modifications were made by A.L. Voskov
 * during the fwfarx256 design, they are intended to turn it into a fast
 * non-cryptographic 64-bit Counter-Based PRNG (CBPRNG):
 *
 * 1. The word size was extended from 32 to 64 bits.
 * 2. Two different rotatations instead of one, dynamic indexing inside
 *    the key schedule was excluded.
 * 3. The key schedule is based on just scrambling the key with a combination
 *    of xorrot LFSR and a non-linear output function.
 * 4. Number of rounds were reduced, also fwfarx256ctr round corresponds to
 *    TWO rounds of the original cipher.
 *
 * Notes about rotations tuning:
 *
 * 1. They were manually tuned by means of hamming_distr test from default and
 *    full batteries of SmokeRand.
 * 2. 2 rounds: hamming_distr from full is ok (even without key addition)
 *    for ctr increment in 0, 1, 2, 3.
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


void fwfarx256_mix_key(uint64_t *mixed_key, const uint64_t *key)
{
    uint64_t x = key[0], y = key[1], z = key[2], w = key[3];
    for (int i = 0; i < 64 + 4; i++) {
        const uint64_t x0 = x, w0 = w;
        x = x0 ^ y;
        y = z;
        z = x0 ^ w0;
        w = (x0 << 3) ^ z ^ rotl64(w0, 8) ^ rotl64(w0, 37);
        if (i >= 64) {
            uint64_t out = rotl64(6906969069U * x0, 11);
            out += (out * out | 0x40000005);
            mixed_key[i - 64] = out;
        }
    }
}

////////////////////////////////////////////////////
///// Scalar (portable) version implementation /////
////////////////////////////////////////////////////

typedef struct {
    uint64_t key[4];
    uint64_t ctr[4];
    uint64_t out[4];
    int pos;
} Fwfarx256CtrState;


static inline void Fwfarx256CtrState_block(Fwfarx256CtrState *obj)
{
    const int sh1 = 35, sh2 = 11;
    uint64_t x = obj->ctr[0], y = obj->ctr[1];
    uint64_t z = obj->ctr[2], w = obj->ctr[3];
    for (int i = 0; i < 3; i++) {
        x += rotl64(y ^ z ^ w, sh1) + obj->key[0];
        y += rotl64(z ^ w ^ x, sh2) + obj->key[1];
        z += rotl64(w ^ x ^ y, sh1) + obj->key[2];
        w += rotl64(x ^ y ^ z, sh2) + obj->key[3];

        x += rotl64(y ^ z ^ w, sh2) + obj->key[0];
        y += rotl64(z ^ w ^ x, sh1) + obj->key[2];
        z += rotl64(w ^ x ^ y, sh2) + obj->key[1];
        w += rotl64(x ^ y ^ z, sh1) + obj->key[3];
    }
    obj->out[0] = x; obj->out[1] = y;
    obj->out[2] = z; obj->out[3] = w;
}

static inline uint64_t get_bits_scalar_raw(Fwfarx256CtrState *obj)
{
    if (obj->pos == 4) {
        obj->ctr[0]++;
        //if (++obj->ctr[0] == 0) obj->ctr[1]++;
        Fwfarx256CtrState_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


void Fwfarx256CtrState_init(Fwfarx256CtrState *obj, const uint64_t *key)
{
    // Key mixer    
    fwfarx256_mix_key(obj->key, key);
    // Reset counter
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    obj->ctr[2] = 0; obj->ctr[3] = 0;
    Fwfarx256CtrState_block(obj);
    obj->pos = 0;
}


static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    uint64_t k[4];
    Fwfarx256CtrState *obj = intf->malloc(sizeof(Fwfarx256CtrState));
    seeds_to_array_u64(intf, k, 4);
    Fwfarx256CtrState_init(obj, k);
    return obj;
}

MAKE_GET_BITS_WRAPPERS(scalar)

////////////////////////////////////////////////
///// AVX2 (vector) version implementation /////
////////////////////////////////////////////////

#define XORMIX256CTR_NCOPIES 8

typedef union {
    uint64_t u64[XORMIX256CTR_NCOPIES];
} Fwfarx256CtrElement;


typedef struct {
    uint64_t key[4];
    Fwfarx256CtrElement ctr[4]; ///< Counter
    Fwfarx256CtrElement out[4]; ///< Output state
    size_t pos;
} Fwfarx256CtrVecState;


#ifdef __AVX2__
static inline void fwfarx256ctr_vec_round(__m256i *s, __m256i *k)
{
    const int sh1 = 35, sh2 = 11;
    // --- Half-round 1 ---
    {
        // s[0] += rotl64(s[1] ^ s[2] ^ s[3], sh1) + k[0];
        __m256i s23 = _mm256_xor_si256(s[2], s[3]);
        s[0] = _mm256_add_epi64(s[0], mm256_rotl_epi64_def(_mm256_xor_si256(s[1], s23), sh1));
        s[0] = _mm256_add_epi64(s[0], k[0]);
        // s[1] += rotl64(s[2] ^ s[3] ^ s[0], sh2) + k[1];
        s[1] = _mm256_add_epi64(s[1], mm256_rotl_epi64_def(_mm256_xor_si256(s23, s[0]), sh2));
        s[1] = _mm256_add_epi64(s[1], k[1]);
        // s[2] += rotl64(s[3] ^ s[0] ^ s[1], sh1) + k[2];
        __m256i s01 = _mm256_xor_si256(s[0], s[1]);
        s[2] = _mm256_add_epi64(s[2], mm256_rotl_epi64_def(_mm256_xor_si256(s[3], s01), sh1));
        s[2] = _mm256_add_epi64(s[2], k[2]);
        // s[3] += rotl64(s[0] ^ s[1] ^ s[2], sh2) + k[3];
        s[3] = _mm256_add_epi64(s[3], mm256_rotl_epi64_def(_mm256_xor_si256(s01, s[2]), sh2));
        s[3] = _mm256_add_epi64(s[3], k[3]);
    }
    // --- Half-round 2 ---
    {
        __m256i s23 = _mm256_xor_si256(s[2], s[3]);
        s[0] = _mm256_add_epi64(s[0], mm256_rotl_epi64_def(_mm256_xor_si256(s[1], s23), sh2));
        s[0] = _mm256_add_epi64(s[0], k[0]);
        s[1] = _mm256_add_epi64(s[1], mm256_rotl_epi64_def(_mm256_xor_si256(s23, s[0]), sh1));
        s[1] = _mm256_add_epi64(s[1], k[2]);
        __m256i s01 = _mm256_xor_si256(s[0], s[1]);
        s[2] = _mm256_add_epi64(s[2], mm256_rotl_epi64_def(_mm256_xor_si256(s[3], s01), sh2));
        s[2] = _mm256_add_epi64(s[2], k[1]);
        s[3] = _mm256_add_epi64(s[3], mm256_rotl_epi64_def(_mm256_xor_si256(s01, s[2]), sh1));
        s[3] = _mm256_add_epi64(s[3], k[3]);
    }
}

static inline void Fwfarx256CtrVecState_block(Fwfarx256CtrVecState *obj)
{
#if XORMIX256CTR_NCOPIES == 4
    __m256i out[4], key[4];
    for (size_t i = 0; i < 4; i++) {
        out[i] = _mm256_loadu_si256((__m256i *) (void *) &obj->ctr[i].u64[0]);
        key[i] = _mm256_set1_epi64x((long long) obj->key[i]);
    }

    for (int i = 0; i < 3; i++) {
        fwfarx256ctr_vec_round(out, key);
    }

    for (size_t i = 0; i < 4; i++) {
        _mm256_storeu_si256((__m256i *) (void *) &obj->out[i].u64[0], out[i]);
    }
#elif XORMIX256CTR_NCOPIES == 8
    __m256i out[8], key[4];
    for (size_t i = 0; i < 4; i++) {
        out[i]     = _mm256_loadu_si256((__m256i *) (void *) &obj->ctr[i].u64[0]);
        out[4 + i] = _mm256_loadu_si256((__m256i *) (void *) &obj->ctr[i].u64[4]);
        key[i] = _mm256_set1_epi64x((long long) obj->key[i]);
    }

    fwfarx256ctr_vec_round(out, key);
    fwfarx256ctr_vec_round(out + 4, key);
    fwfarx256ctr_vec_round(out, key);
    fwfarx256ctr_vec_round(out + 4, key);
    fwfarx256ctr_vec_round(out, key);
    fwfarx256ctr_vec_round(out + 4, key);

    for (size_t i = 0; i < 4; i++) {
        _mm256_storeu_si256((__m256i *) (void *) &obj->out[i].u64[0], out[i]);
        _mm256_storeu_si256((__m256i *) (void *) &obj->out[i].u64[4], out[4 + i]);
    }
#else
    #error "This number of copies is not supported"
#endif
}

void Fwfarx256CtrVecState_init(Fwfarx256CtrVecState *obj, const uint64_t *key)
{
    // Key mixer    
    fwfarx256_mix_key(obj->key, key);
    // Reset counter
    for (size_t i = 0; i < XORMIX256CTR_NCOPIES; i++) {
        obj->ctr[0].u64[i] = i; obj->ctr[1].u64[i] = 0;
        obj->ctr[2].u64[i] = 0; obj->ctr[3].u64[i] = 0;
    }
    Fwfarx256CtrVecState_block(obj);
    obj->pos = 0;
}


static inline uint64_t get_bits_vector_raw(Fwfarx256CtrVecState *obj)
{
    const size_t i = (obj->pos & 0x3), j = (obj->pos >> 2);
    const uint64_t x = obj->out[i].u64[j];
    obj->pos++;
    if (obj->pos == 4 * XORMIX256CTR_NCOPIES) {
        for (size_t k = 0; k < XORMIX256CTR_NCOPIES; k++) {
            obj->ctr[0].u64[k] += XORMIX256CTR_NCOPIES;
        }
        Fwfarx256CtrVecState_block(obj);
        obj->pos = 0;
    }
    return x;
}



static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    uint64_t k[4];
    Fwfarx256CtrVecState *obj = intf->malloc(sizeof(Fwfarx256CtrVecState));
    seeds_to_array_u64(intf, k, 4);
    Fwfarx256CtrVecState_init(obj, k);
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
    {"",     "fwfarx256ctr:c99",  64, create_scalar, get_bits_scalar, get_sum_scalar},
    {"c99",  "fwfarx256ctr:c99",  64, create_scalar, get_bits_scalar, get_sum_scalar},
#ifdef __AVX2__
    {"avx2", "fwfarx256ctr:avx2", 64, create_vector, get_bits_vector, get_sum_vector},
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
    static const uint64_t k[4] = {
        0x243F6A8885A308D3, 0x13198A2E03707344,
        0xA4093822299F31D0, 0x082EFA98EC4E6C89
    };
    static const uint64_t u_ref[16] = {
        0xBF6D9F040BB0B616, 0x25886D866545E424,
        0x72053BE8A5CEE68C, 0xBFEDAB2CAA52FB06,
        0x6ED2A21E68A7D996, 0xDCC38069AC5BA507,
        0x23571484E3386A44, 0x90826208856B7635,
        0x47540D923DEE0BAD, 0xEE6DAE54BA496EFD,
        0x0F8371960E128DA7, 0x7D886ED0156B850B,
        0x1B9FA676707B3A4C, 0xC8955F6749011CF4,
        0xEB9F38757A11C286, 0x04C4897C6C6CC481
    };
    int is_ok = 1;
    Fwfarx256CtrState *obj = intf->malloc(sizeof(Fwfarx256CtrState));
    Fwfarx256CtrState_init(obj, k);
    for (int i = 0; i < 16; i++) {
        const uint64_t u = get_bits_scalar_raw(obj);
        intf->printf("0x%16.16llX 0x%16.16llX\n",
            (unsigned long long) u,
            (unsigned long long) u_ref[i]
        );
        if (u_ref[i] != u) {
            is_ok = 0;
        }
    }

#ifdef __AVX2__
    Fwfarx256CtrVecState *obj_vec = intf->malloc(sizeof(Fwfarx256CtrVecState));
    Fwfarx256CtrVecState_init(obj_vec, k);
    for (int i = 0; i < 16; i++) {
        (void) get_bits_vector_raw(obj_vec);
    }
    for (int i = 0; i < 32768; i++) {
        if (get_bits_vector_raw(obj_vec) != get_bits_scalar_raw(obj)) {
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
"fwfarx256ctr is the counter-based PRNG with 256-bit block.\n"
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
