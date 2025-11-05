/**
 * @file xtea.c
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

#ifdef  __AVX2__
#define XTEA_VEC_ENABLED
#endif

#ifdef XTEA_VEC_ENABLED
#include "smokerand/x86exts.h"
#endif

PRNG_CMODULE_PROLOG

/**
 * @brief XTEA PRNG state.
 */
typedef struct {
    uint64_t ctr;
    uint32_t key[4];
} XteaState;

/**
 * @brief Number of generator copies inside the state.
 */
#define XTEA_NCOPIES 16

/**
 * @brief XTEA vectorized PRNG state. It contains 16 copies of XTEA and can work
 * either in CTR or CBC operation mode.
 * @details The next layout is used for both input (plaintext) and output
 * (ciphertext):
 *
 * \f[
 * \left[ x^{low}_0, \ldots, x^{low}_{15}, x^{high}_0,\ldots,x^{high}_{15} \right]
 * \f]
 */
typedef struct {
    uint32_t in[XTEA_NCOPIES * 2]; ///< Counters (plaintext).
    uint32_t out[XTEA_NCOPIES * 2]; ///< Output (ciphertext).
    uint32_t key[4]; ///< 256-bit key.
    int pos; ///< Current position in the buffer (from 0 to 15).
    int is_cbc; ///< 0/1 - CTR/CBC operation mode.
} XteaVecState;

/////////////////////////////////////////
///// Scalar version implementation /////
/////////////////////////////////////////


static inline uint64_t get_bits_scalar_raw(void *state)
{
    static const uint32_t DELTA = 0x9e3779b9;
    XteaState *obj = state;
    uint32_t sum = 0, y, z;
    union {
        uint32_t v[2];
        uint64_t x;
    } out;
    out.x = obj->ctr++;
    y = out.v[0];
    z = out.v[1];
    for (int i = 0; i < 32; i++) {
        y += ( ((z<<4) ^ (z>>5)) + z) ^ (sum + obj->key[sum & 3]);
        sum += DELTA;
        z += ( ((y<<4) ^ (y>>5)) + y) ^ (sum + obj->key[(sum >> 11) & 3]);
    }
    out.v[0] = y;
    out.v[1] = z;
    return out.x;
}

MAKE_GET_BITS_WRAPPERS(scalar)

static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    XteaState *obj = intf->malloc(sizeof(XteaState));
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    obj->key[0] = (uint32_t) s0;
    obj->key[1] = (uint32_t) (s0 >> 32);
    obj->key[2] = (uint32_t) s1;
    obj->key[3] = (uint32_t) (s1 >> 32);
    obj->ctr = 0;
    (void) gi;
    return obj;
}

static int run_self_test_scalar(const CallerAPI *intf)
{
    XteaState obj = {.ctr = 0x547571AAAF20A390,
        .key = {0x27F917B1, 0xC1DA8993, 0x60E2ACAA, 0xA6EB923D}};
    uint64_t u_ref = 0x0A202283D26428AF;
    uint64_t u = get_bits_scalar_raw(&obj);
    intf->printf("----- Scalar version self-test -----\n");
    intf->printf("Results: out = %llX; ref = %llX\n",
        (unsigned long long) u,
        (unsigned long long) u_ref);
    return u == u_ref;    
}

/////////////////////////////////////////////
///// Vectorized version implementation /////
/////////////////////////////////////////////

#ifdef XTEA_VEC_ENABLED
static inline __m256i mix(__m256i x, __m256i key)
{
    return _mm256_xor_si256(key, _mm256_add_epi32(
        x,
        _mm256_xor_si256(_mm256_slli_epi32(x, 4),
        _mm256_srli_epi32(x, 5))
    ));
}
#endif

/**
 * @brief XTEA encryption function (a vectorized version).
 * @relates XteaVecState
 */
void XteaVecState_block(XteaVecState *obj)
{
#ifdef XTEA_VEC_ENABLED
    static const uint32_t DELTA = 0x9e3779b9;
    uint32_t sum = 0;
    __m256i y_a = _mm256_loadu_si256((__m256i *) (void *) obj->in);
    __m256i y_b = _mm256_loadu_si256((__m256i *) (void *) (obj->in + 8));
    __m256i z_a = _mm256_loadu_si256((__m256i *) (void *) (obj->in + 16));
    __m256i z_b = _mm256_loadu_si256((__m256i *) (void *) (obj->in + 24));
    // CBC mode: XOR input (counter) with the previous input
    if (obj->is_cbc) {
        __m256i out0_a = _mm256_loadu_si256((__m256i *) (void *) obj->out);
        __m256i out0_b = _mm256_loadu_si256((__m256i *) (void *) (obj->out + 8));
        __m256i out1_a = _mm256_loadu_si256((__m256i *) (void *) (obj->out + 16));
        __m256i out1_b = _mm256_loadu_si256((__m256i *) (void *) (obj->out + 24));
        y_a = _mm256_xor_si256(y_a, out0_a);
        y_b = _mm256_xor_si256(y_b, out0_b);
        z_a = _mm256_xor_si256(z_a, out1_a);
        z_b = _mm256_xor_si256(z_b, out1_b);
    }
    for (int i = 0; i < 32; i++) {
        __m256i keyA = _mm256_set1_epi32((int) (sum + obj->key[sum & 3]));
        y_a = _mm256_add_epi32(y_a, mix(z_a, keyA));
        y_b = _mm256_add_epi32(y_b, mix(z_b, keyA));
        sum += DELTA;
        __m256i keyB = _mm256_set1_epi32((int) (sum + obj->key[(sum >> 11) & 3]));
        z_a = _mm256_add_epi32(z_a, mix(y_a, keyB));
        z_b = _mm256_add_epi32(z_b, mix(y_b, keyB));
    }
    _mm256_storeu_si256((__m256i *) (void *) obj->out, y_a);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 8), y_b);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 16), z_a);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 24), z_b);
#else
    (void) obj;
#endif
}

/**
 * @brief Initializes an example of XTEA vectorized PRNG.
 * @param obj Pointer to the generator to be initialized.
 * @param key 256-bit key.
 * @relates XteaVecState
 */
void XteaVecState_init(XteaVecState *obj, const uint32_t *key)
{
    for (size_t i = 0; i < 4; i++) {
        obj->key[i] = key[i];
    }
    for (unsigned int i = 0; i < XTEA_NCOPIES; i++) {
        obj->in[i] = i;
    }
    for (size_t i = XTEA_NCOPIES; i < 2 * XTEA_NCOPIES; i++) {
        obj->in[i] = 0;
    }
    for (size_t i = 0; i < 2 * XTEA_NCOPIES; i++) {
        obj->out[i] = 0; // Needed for CBC mode
    }
    obj->pos = XTEA_NCOPIES;
    obj->is_cbc = 0;
}

/**
 * @brief Increase internal counters. There are 8 64-bit counters in the AVX2
 * version of the XTEA based PRNG.
 * @relates XteaVecState
 */
static inline void XteaVecState_inc_ctr(XteaVecState *obj)
{
    for (int i = 0; i < XTEA_NCOPIES; i++) {
        obj->in[i] += XTEA_NCOPIES;
    }
    // 32-bit counters overflow: increment the upper halves
    if (obj->in[0] == 0) {
        for (int i = XTEA_NCOPIES; i < 2 * XTEA_NCOPIES; i++) {
            obj->in[i]++;
        }
    }
}


static inline uint64_t get_bits_vector_raw(void *state)
{
    XteaVecState *obj = state;
    if (obj->pos >= XTEA_NCOPIES) {
        XteaVecState_block(obj);
        XteaVecState_inc_ctr(obj);
        obj->pos = 0;
    }
    uint64_t val = obj->out[obj->pos];
    val |= ((uint64_t) obj->out[obj->pos + XTEA_NCOPIES]) << 32;
    obj->pos++;
    return val;
}


MAKE_GET_BITS_WRAPPERS(vector)

static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
    XteaVecState *obj = intf->malloc(sizeof(XteaVecState));
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    uint32_t key[4];
    key[0] = (uint32_t) s0; key[1] = (uint32_t) (s0 >> 32);
    key[2] = (uint32_t) s1; key[3] = (uint32_t) (s1 >> 32);
    XteaVecState_init(obj, key);
    const char *mode_name = intf->get_param();
    if (!intf->strcmp(mode_name, "vector-ctr") || !intf->strcmp(mode_name, "")) {
        obj->is_cbc = 0;
    } else if (!intf->strcmp(mode_name, "vector-cbc")) {
        obj->is_cbc = 1;
    } else {
        intf->printf("Unknown operation mode '%s' (vector-ctr or vector-cbc are supported)",
            mode_name);
        intf->free(obj);
        return NULL;
    }
    (void) gi;
    return obj;
}

static int run_self_test_vector(const CallerAPI *intf)
{
#ifdef XTEA_VEC_ENABLED
    XteaVecState *obj = intf->malloc(sizeof(XteaVecState));
    const uint64_t u_ref = 0x0A202283D26428AF;
    const uint32_t key[4] = {0x27F917B1, 0xC1DA8993, 0x60E2ACAA, 0xA6EB923D};
    uint64_t u;
    XteaVecState_init(obj, key);
    for (int i = 0; i < XTEA_NCOPIES; i++) {
        obj->in[i]                = 0xAF20A390;
        obj->in[i + XTEA_NCOPIES] = 0x547571AA;
    }
    for (int i = 0; i < XTEA_NCOPIES; i++) {
        u = get_bits_vector_raw(obj);
    }
    intf->printf("----- Vectorized version self-test -----\n");
    intf->printf("Results: out = %llX; ref = %llX\n",
        (unsigned long long) u,
        (unsigned long long) u_ref);
    intf->free(obj);
    return u == u_ref;
#else
    intf->printf("----- Vectorized version is not supported on this platform -----\n");
    return 1;
#endif
}

//////////////////////
///// Interfaces /////
//////////////////////


static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    is_ok = is_ok & run_self_test_scalar(intf);
    is_ok = is_ok & run_self_test_vector(intf);
    return is_ok;
}

static void *create(const CallerAPI *intf)
{
    (void) intf;
    return NULL;
}


static const char description[] = 
"This generator is based on the XTEA block cipher; param values:\n"
"- scalar-ctr - run the scalar version in the CTR mode (default).\n"
"- vector-ctr - run the vectorized (AVX2) version in the CTR mode.\n"
"- vector-cbc - run the vectorized (AVX2) version in the CBC mode.\n"
"The CTR versions fail 64-bit birthday paradox test\n";

int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 64;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "scalar-ctr") || !intf->strcmp(param, "")) {
        gi->name = "XTEA:scalar-ctr";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "vector-ctr")) {
        gi->name = "XTEA:vector-ctr";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else if (!intf->strcmp(param, "vector-cbc")) {
        gi->name = "XTEA:vector-cbc";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else {
        gi->name = "XTEA:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
