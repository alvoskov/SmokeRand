// 
// Hars, L., Petruska, G. Pseudorandom recursions II. J Embedded Systems 2012, 1 (2012). https://doi.org/10.1186/1687-3963-2012-1
// Hars, L., Petruska, G. Pseudorandom Recursions: Small and Fast Pseudorandom Number Generators for Embedded Applications. J Embedded Systems 2007, 098417 (2007). https://doi.org/10.1155/2007/98417
// Hars L. Hardware Bit-Mixers. Cryptology {ePrint} Archive, Paper 2017/084. https://eprint.iacr.org/2017/084}
#include "smokerand/cinterface.h"

#ifdef __AVX2__
#include "smokerand/x86exts.h"
#endif

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t key[4];
    uint64_t ctr[4];
    uint64_t out[4];
    int pos;
} Xormix256CtrState;

// 4 rounds: i < 2: hamming_distr from full is ok (even without key schedule)
// for ctr 0, 1, 2, 3(!);
// ctr[0] passes the entire "full" SmokeRand
// and >= 1 TiB in PractRand 0.96
static inline void Xormix256CtrState_block(Xormix256CtrState *obj)
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

static inline uint64_t get_bits_scalar_raw(Xormix256CtrState *obj)
{
    if (obj->pos == 4) {
        obj->ctr[0]++;
        //if (++obj->ctr[0] == 0) obj->ctr[1]++;
        Xormix256CtrState_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


void xormix256_mix_key(uint64_t *mixed_key, const uint64_t *key)
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

void Xormix256CtrState_init(Xormix256CtrState *obj, const uint64_t *key)
{
    // Key mixer    
    xormix256_mix_key(obj->key, key);
    // Reset counter
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    obj->ctr[2] = 0; obj->ctr[3] = 0;
    Xormix256CtrState_block(obj);
    obj->pos = 0;
}


static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    uint64_t k[4];
    Xormix256CtrState *obj = intf->malloc(sizeof(Xormix256CtrState));
    seeds_to_array_u64(intf, k, 4);
    Xormix256CtrState_init(obj, k);
    return obj;
}

MAKE_GET_BITS_WRAPPERS(scalar)

////////////////////////////////////////
///// AVX2 (vector) implementation /////
////////////////////////////////////////

#define XORMIX256CTR_NCOPIES 4

typedef union {
    uint64_t u64[XORMIX256CTR_NCOPIES];
} Xormix256CtrElement;


typedef struct {
    uint64_t key[4];
    Xormix256CtrElement ctr[4]; ///< Counter
    Xormix256CtrElement out[4]; ///< Output state
    size_t pos;
} Xormix256CtrVecState;


static inline void xormix256ctr_vec_round(__m256i *s, __m256i *k)
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


static inline void Xormix256CtrVecState_block(Xormix256CtrVecState *obj)
{
    __m256i out[4], key[4];
    for (size_t i = 0; i < 4; i++) {
        out[i] = _mm256_loadu_si256((__m256i *) (void *) &obj->ctr[i].u64[0]);
        key[i] = _mm256_set1_epi64x((long long) obj->key[i]);
    }

    for (int i = 0; i < 3; i++) {
        xormix256ctr_vec_round(out, key);
    }

    for (size_t i = 0; i < 4; i++) {
        _mm256_storeu_si256((__m256i *) (void *) &obj->out[i].u64[0], out[i]);
    }

    (void) obj;
}


void Xormix256CtrVecState_init(Xormix256CtrVecState *obj, const uint64_t *key)
{
    // Key mixer    
    xormix256_mix_key(obj->key, key);
    // Reset counter
    for (size_t i = 0; i < XORMIX256CTR_NCOPIES; i++) {
        obj->ctr[0].u64[i] = i; obj->ctr[1].u64[i] = 0;
        obj->ctr[2].u64[i] = 0; obj->ctr[3].u64[i] = 0;
    }
    Xormix256CtrVecState_block(obj);
    obj->pos = 0;
}


static inline uint64_t get_bits_vector_raw(Xormix256CtrVecState *obj)
{
    const size_t i = (obj->pos & 0x3), j = (obj->pos >> 2);
    const uint64_t x = obj->out[i].u64[j];
    obj->pos++;
    if (obj->pos == 4 * XORMIX256CTR_NCOPIES) {
        for (size_t k = 0; k < XORMIX256CTR_NCOPIES; k++) {
            obj->ctr[0].u64[k] += XORMIX256CTR_NCOPIES;
        }
        Xormix256CtrVecState_block(obj);
        obj->pos = 0;
    }
    return x;
}



static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    uint64_t k[4];
    Xormix256CtrVecState *obj = intf->malloc(sizeof(Xormix256CtrVecState));
    seeds_to_array_u64(intf, k, 4);
    Xormix256CtrVecState_init(obj, k);
    return obj;
}

MAKE_GET_BITS_WRAPPERS(vector)




//////////////////////
///// Interfaces /////
//////////////////////

static void *create(const CallerAPI *intf)
{
    intf->printf("'%s' not implemented\n", intf->get_param());
    return NULL;
}

static const GeneratorParamVariant gen_list[] = {
    {"",     "xormix256ctr:c99",  64, create_scalar, get_bits_scalar, get_sum_scalar},
    {"c99",  "xormix256ctr:c99",  64, create_scalar, get_bits_scalar, get_sum_scalar},
    {"avx2", "xormix256ctr:avx2", 64, create_vector, get_bits_vector, get_sum_vector},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static int run_self_test(const CallerAPI *intf)
{
    const uint64_t k[4] = {0x123456789ABCDEF, 0xFEDCBA987654321, 3, 4};
    int is_ok = 1;
    Xormix256CtrState *obj = intf->malloc(sizeof(Xormix256CtrState));
    Xormix256CtrState_init(obj, k);
    for (int i = 0; i < 16; i++) {
        intf->printf("%llX\n", get_bits_scalar_raw(obj));
    }

    Xormix256CtrVecState *obj_vec = intf->malloc(sizeof(Xormix256CtrVecState));
    Xormix256CtrVecState_init(obj_vec, k);
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
    intf->free(obj);



    return is_ok;
    
}


static const char description[] =
"xormix256ctr is the counter-based PRNG with 256-bit block.\n"
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
