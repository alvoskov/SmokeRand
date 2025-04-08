/**
 * @file xtea.c
 * @brief An implementation of PRNG based on XXTEA with 128-bit and 256-bit
 * blocks. Contains both scalar and AVX2 versions.
 * @details XXTEA is used as a "lightweight cryptography" for embedded systems.
 * Blocks longer than 256 bits should be avoided due to the vulnerability
 * described by E. Yarrkov for the 6-round version of XXTEA [2].
 *
 * References:
 * 1. Wheeler D.J., Needham R.M. Correction to XTEA.
 *    https://www.movable-type.co.uk/scripts/xxtea.pdf
 * 2. Yarrkov E. Cryptanalysis of XXTEA // Cryptology ePrint Archive,
 *    Paper 2010/254. 2010. https://eprint.iacr.org/2010/254
 * 3. Ma Bingyao Crypt-XXTEA CPAN Perl module.
 *    https://github.com/an0maly/Crypt-XXTEA/blob/master/reference/test-vector.t
 *
 * Testing of XXTEA with 128-bit block:
 *
 * - 2 rounds: fails `express` battery.
 * - 3 rounds: fails `full` battery, `sumcollector` test.
 * - 4 rounds: passes `full` battery.
 *
 * Testing of XXTEA with 256-bit block (AVX):
 *
 * - 1 round:  fails `express` battery.
 * - 2 rounds: passes `express` battery, fails `default` battery;
 *   fails 8/16-bit frequency tests, `gap16_count0` and `matrixrank_4096_low8`
 *   (but not `linearcomp`!) tests. Also fails `brief` battery.
 * - 3 rounds: passes `full` battery.
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
#include "smokerand/x86exts.h"
#endif

enum {
    XXTEA_DELTA = 0x9e3779b9, ///< Increment for round keys.
    XXTEA_NCOPIES = 8, ///< Number of generator copies inside the state.
    XXTEA128_NROUNDS = 19, ///< Number of rounds for 128-bit blocks.
    XXTEA256_NROUNDS = 12, ///< Number of rounds for 256-bit blocks.
};

typedef enum {
    XXTEA_UNKNOWN,
    XXTEA128_SCALAR,
    XXTEA128_AVX2,
    XXTEA256_SCALAR,
    XXTEA256_AVX2
} XxteaType;


typedef struct {
    XxteaType type;
    const char *name;
} XxteaInfo;


static XxteaInfo XxteaInfo_get(const CallerAPI *intf)
{
    const char *ver = intf->get_param();
    if (!intf->strcmp(ver, "128-scalar") || !intf->strcmp(ver, "")) {
        XxteaInfo info = {XXTEA128_SCALAR, "XXTEA128-scalar"};
        return info;
    } else if (!intf->strcmp(ver, "256-scalar")) {
        XxteaInfo info = {XXTEA256_SCALAR, "XXTEA256-scalar"};
        return info;
    } else if (!intf->strcmp(ver, "128-avx2")) {
        XxteaInfo info = {XXTEA128_AVX2, "XXTEA128-avx2"};
        return info;
    } else if (!intf->strcmp(ver, "256-avx2")) {
        XxteaInfo info = {XXTEA256_AVX2, "XXTEA256-avx2"};
        return info;
    } else {
        XxteaInfo info = {XXTEA_UNKNOWN, "XXTEA:unknown"};
        return info;
    }
}


/**
 * @brief XXTEA128 PRNG state: scalar version.
 */
typedef struct {
    BufGen32Interface intf;
    uint32_t key[4]; ///< 128-bit key
    uint32_t ctr[4]; ///< Counter (plaintext)
    uint32_t out[4]; ///< Output buffer (ciphertext)
} Xxtea128State;

/**
 * @brief XXTEA128 PRNG state: vectorized (AVX2) version.
 */
typedef struct {
    BufGen32Interface intf;
    uint32_t key[4]; ///< 128-bit key
    uint32_t ctr[4 * XXTEA_NCOPIES]; ///< Counter (plaintext)
    uint32_t out[4 * XXTEA_NCOPIES]; ///< Output buffer (ciphertext)
} Xxtea128VecState;


/**
 * @brief XXTEA256 PRNG state: scalar version.
 */
typedef struct {
    BufGen32Interface intf;
    uint32_t key[4]; ///< 128-bit key
    uint32_t ctr[8]; ///< Counter (plaintext)
    uint32_t out[8]; ///< Output buffer (ciphertext)
} Xxtea256State;

/**
 * @brief XXTEA256 PRNG state: vectorized (AVX2) version.
 */
typedef struct {
    BufGen32Interface intf;
    uint32_t key[4]; ///< 128-bit key
    uint32_t ctr[8 * XXTEA_NCOPIES]; ///< Counter (plaintext)
    uint32_t out[8 * XXTEA_NCOPIES]; ///< Output buffer (ciphertext)
} Xxtea256VecState;

////////////////////////////
///// Mixing functions /////
////////////////////////////

/**
 * @brief XXTEA mixing function: scalar version.
 */
static inline uint32_t mix(uint32_t y, uint32_t z, uint32_t sum, uint32_t rk)
{
    return (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (rk ^ z)));
}

#ifdef XXTEA_VEC_ENABLED
/**
 * @brief XXTEA mixing function: vectorized (AVX2) version.
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
#endif



//////////////////////////////////////////////
///// Xxtea128State class implementation /////
//////////////////////////////////////////////

/**
 * @brief XXTEA encryption subroutine.
 */
void Xxtea128State_block(Xxtea128State *obj)
{
    uint32_t y = obj->ctr[0], z = obj->ctr[3], sum = 0;
    for (int i = 0; i < 4; i++) {
        obj->out[i] = obj->ctr[i];
    }
    for (int i = 0; i < XXTEA128_NROUNDS; i++) {
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
static inline void Xxtea128State_inc_counter(Xxtea128State *obj)
{
    uint64_t *ctr = (uint64_t *) obj->ctr;
    (*ctr)++;
}

/**
 * @brief Generates a new block of pseudorandom numbers and updates
 * internal counters.
 * @relates LeaState.
 */
void Xxtea128State_iter_func(void *data)
{
    Xxtea128State *obj = data;
    Xxtea128State_block(obj);
    Xxtea128State_inc_counter(obj);
    obj->intf.pos = 0;
}

/**
 * @brief Initializes an example of XXTEA scalar PRNG with 128-bit block.
 * @param obj Pointer to the generator to be initialized.
 * @param key 128-bit key.
 * @relates XteaVecState
 */
void Xxtea128State_init(Xxtea128State *obj, const uint32_t *key)
{
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key[i];
        obj->ctr[i] = 0;
    }
    obj->intf.iter_func = Xxtea128State_iter_func;
    obj->intf.pos = 4;
    obj->intf.bufsize = 4;
    obj->intf.out = obj->out;
}



/////////////////////////////////////////////////
///// Xxtea128VecState class implementation /////
/////////////////////////////////////////////////


/**
 * @brief XXTEA encryption subroutine.
 */
void Xxtea128VecState_block(Xxtea128VecState *obj)
{
#ifdef XXTEA_VEC_ENABLED
    uint32_t sum = 0;
    __m256i out[4], y, z, keyv[4];
    out[0] = _mm256_loadu_si256((__m256i *) obj->ctr);
    out[1] = _mm256_loadu_si256((__m256i *) (obj->ctr + XXTEA_NCOPIES));
    out[2] = _mm256_loadu_si256((__m256i *) (obj->ctr + 2*XXTEA_NCOPIES));
    out[3] = _mm256_loadu_si256((__m256i *) (obj->ctr + 3*XXTEA_NCOPIES));
    keyv[0] = _mm256_set1_epi32(obj->key[0]);
    keyv[1] = _mm256_set1_epi32(obj->key[1]);
    keyv[2] = _mm256_set1_epi32(obj->key[2]);
    keyv[3] = _mm256_set1_epi32(obj->key[3]);
    y = out[0], z = out[3];
    for (int i = 0; i < XXTEA128_NROUNDS; i++) {
        sum += XXTEA_DELTA;
        unsigned int e = (sum >> 2) & 3;
        __m256i sumv = _mm256_set1_epi32(sum);
        y = out[1];
        z = _mm256_add_epi32(out[0], mixv(y, z, sumv, keyv[0 ^ e])); out[0] = z;
        y = out[2];
        z = _mm256_add_epi32(out[1], mixv(y, z, sumv, keyv[1 ^ e])); out[1] = z;
        y = out[3];
        z = _mm256_add_epi32(out[2], mixv(y, z, sumv, keyv[2 ^ e])); out[2] = z;
        y = out[0];
        z = _mm256_add_epi32(out[3], mixv(y, z, sumv, keyv[3 ^ e])); out[3] = z;
    }
    _mm256_storeu_si256((__m256i *) obj->out,                     out[0]);
    _mm256_storeu_si256((__m256i *) (obj->out + XXTEA_NCOPIES),   out[1]);
    _mm256_storeu_si256((__m256i *) (obj->out + 2*XXTEA_NCOPIES), out[2]);
    _mm256_storeu_si256((__m256i *) (obj->out + 3*XXTEA_NCOPIES), out[3]);
#else
    (void) obj;
#endif
}


/**
 * @brief Increase the internal 64-bit counter.
 */
static inline void Xxtea128VecState_inc_counter(Xxtea128VecState *obj)
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
void Xxtea128VecState_iter_func(void *data)
{
    Xxtea128VecState *obj = data;
    Xxtea128VecState_block(obj);
    Xxtea128VecState_inc_counter(obj);
    obj->intf.pos = 0;
}

/**
 * @brief Initializes an example of XXTEA vectorized PRNG.
 * @param obj Pointer to the generator to be initialized.
 * @param key 256-bit key.
 * @relates XteaVecState
 */
void Xxtea128VecState_init(Xxtea128VecState *obj, const uint32_t *key)
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
    obj->intf.iter_func = Xxtea128VecState_iter_func;
    obj->intf.pos = 4 * XXTEA_NCOPIES;
    obj->intf.bufsize = 4 * XXTEA_NCOPIES;
    obj->intf.out = obj->out;
}

//////////////////////////////////////////////
///// Xxtea256State class implementation /////
//////////////////////////////////////////////

/**
 * @brief XXTEA encryption subroutine.
 */
void Xxtea256State_block(Xxtea256State *obj)
{
    uint32_t y = obj->ctr[0], z = obj->ctr[7], sum = 0;
    for (int i = 0; i < 8; i++) {
        obj->out[i] = obj->ctr[i];
    }
    for (int i = 0; i < XXTEA256_NROUNDS; i++) {
        sum += XXTEA_DELTA;
        unsigned int e = (sum >> 2) & 3;
        for (int j = 0; j < 7; j++) {
            y = obj->out[j + 1]; 
            z = obj->out[j] += mix(y, z, sum, obj->key[(j & 3) ^ e]);
        }
        y = obj->out[0];
        z = obj->out[7] += mix(y, z, sum, obj->key[(7 & 3) ^ e]);
    }
}

/**
 * @brief Increase the internal 64-bit counter.
 */
static inline void Xxtea256State_inc_counter(Xxtea256State *obj)
{
    uint64_t *ctr = (uint64_t *) obj->ctr;
    (*ctr)++;
}

/**
 * @brief Generates a new block of pseudorandom numbers and updates
 * internal counters.
 * @relates LeaState.
 */
void Xxtea256State_iter_func(void *data)
{
    Xxtea256State *obj = data;
    Xxtea256State_block(obj);
    Xxtea256State_inc_counter(obj);
    obj->intf.pos = 0;
}

/**
 * @brief Initializes an example of XXTEA scalar PRNG with 256-bit block.
 * @param obj Pointer to the generator to be initialized.
 * @param key 128-bit key.
 * @relates XteaVecState
 */
void Xxtea256State_init(Xxtea256State *obj, const uint32_t *key)
{
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key[i];
    }
    for (int i = 0; i < 8; i++) {
        obj->ctr[i] = 0;
    }
    obj->intf.iter_func = Xxtea256State_iter_func;
    obj->intf.pos = 8;
    obj->intf.bufsize = 8;
    obj->intf.out = obj->out;
}

/////////////////////////////////////////////////
///// Xxtea256VecState class implementation /////
/////////////////////////////////////////////////

/**
 * @brief XXTEA encryption subroutine.
 */
void Xxtea256VecState_block(Xxtea256VecState *obj)
{
#ifdef XXTEA_VEC_ENABLED
    uint32_t sum = 0;
    __m256i out[8], y, z;
    for (int i = 0; i < 8; i++) {
        out[i] = _mm256_loadu_si256((__m256i *) (obj->ctr + i*XXTEA_NCOPIES));
    }
    y = out[0], z = out[7];
    for (int i = 0; i < XXTEA256_NROUNDS; i++) {
        sum += XXTEA_DELTA;
        unsigned int e = (sum >> 2) & 3;
        __m256i sumv = _mm256_set1_epi32(sum), rk;
        for (int j = 0; j < 7; j++) {
            __m256i rk = _mm256_set1_epi32(obj->key[(j & 3) ^ e]);
            y = out[j + 1];
            z = _mm256_add_epi32(out[j], mixv(y, z, sumv, rk)); out[j] = z;
        }
        rk = _mm256_set1_epi32(obj->key[(7 & 3) ^ e]);
        y = out[0];
        z = _mm256_add_epi32(out[7], mixv(y, z, sumv, rk)); out[7] = z;
    }
    for (int i = 0; i < 8; i++) {
        _mm256_storeu_si256((__m256i *) (obj->out + i*XXTEA_NCOPIES), out[i]);
    }
#else
    (void) obj;
#endif
}


/**
 * @brief Increase the internal 64-bit counter.
 */
static inline void Xxtea256VecState_inc_counter(Xxtea256VecState *obj)
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
void Xxtea256VecState_iter_func(void *data)
{
    Xxtea256VecState *obj = data;
    Xxtea256VecState_block(obj);
    Xxtea256VecState_inc_counter(obj);
    obj->intf.pos = 0;
}

/**
 * @brief Initializes an example of XXTEA vectorized PRNG.
 * @param obj Pointer to the generator to be initialized.
 * @param key 128-bit key.
 * @relates XteaVecState
 */
void Xxtea256VecState_init(Xxtea256VecState *obj, const uint32_t *key)
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
    obj->intf.iter_func = Xxtea256VecState_iter_func;
    obj->intf.pos = 8 * XXTEA_NCOPIES;
    obj->intf.bufsize = 8 * XXTEA_NCOPIES;
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
    XxteaInfo info = XxteaInfo_get(intf);
    if (info.type == XXTEA128_SCALAR) {
        Xxtea128State *obj = intf->malloc(sizeof(Xxtea128State));
        Xxtea128State_init(obj, key);
        return obj;
    } else if (info.type == XXTEA256_SCALAR) {
        Xxtea256State *obj = intf->malloc(sizeof(Xxtea256State));
        Xxtea256State_init(obj, key);
        return obj;
    } else if (info.type == XXTEA128_AVX2) {
#ifdef XXTEA_VEC_ENABLED
        Xxtea128VecState *obj = intf->malloc(sizeof(Xxtea128VecState));
        Xxtea128VecState_init(obj, key);
        return obj;
#else
        intf->printf("AVX2 is not supported on this platform\n");
        return NULL;
#endif
    } else if (info.type == XXTEA256_AVX2) {
#ifdef XXTEA_VEC_ENABLED
        Xxtea256VecState *obj = intf->malloc(sizeof(Xxtea256VecState));
        Xxtea256VecState_init(obj, key);
        return obj;
#else
        intf->printf("AVX2 is not supported on this platform\n");
        return NULL;
#endif
    } else {
        intf->printf("Unknown version '%s' (128-scalar, 256-scalar, 128-avx2 or 256-avx2 are supported)\n",
            intf->get_param());
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


int scalar_test(const CallerAPI *intf)
{
    static const uint32_t
        key[4]  = {0xb979379e, 0xe973979b, 0x9e3779b9, 0x5651696b},
        in[4]   = {0x08040201, 0x80402010, 0xf8fcfeff, 0x80c0e0f0},
        ref[4]  = {0xfd15b801, 0xd194482e, 0x43da5535, 0x8a869d4c},
        key2[4] = {0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa},
        ref2[4] = {0x069fa1c0, 0x39d6b0eb, 0xf727aa25, 0xd0b2c64c};

    Xxtea128State *obj = intf->malloc(sizeof(Xxtea128State));
    int is_ok = 1;
    // Test 1
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key[i];
        obj->ctr[i] = in[i];
    }
    Xxtea128State_block(obj);
    is_ok = is_ok & cmp_vec4(intf, obj->out, ref);
    // Test 2
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key2[i];
    }
    Xxtea128State_block(obj);
    is_ok = is_ok & cmp_vec4(intf, obj->out, ref2);
    intf->free(obj);
    return is_ok;
}


int vector_test(const CallerAPI *intf)
{
    static const uint32_t
        key[4]  = {0xb979379e, 0xe973979b, 0x9e3779b9, 0x5651696b},
        in[4]   = {0x08040201, 0x80402010, 0xf8fcfeff, 0x80c0e0f0},
        ref[4]  = {0xfd15b801, 0xd194482e, 0x43da5535, 0x8a869d4c};
    int is_ok = 1;
    Xxtea128VecState *obj = intf->malloc(sizeof(Xxtea128VecState));
    Xxtea128VecState_init(obj, key);
    // Test 1
    for (int i = 0; i < XXTEA_NCOPIES; i++) {
        obj->ctr[i]                   = in[0];
        obj->ctr[i + XXTEA_NCOPIES]   = in[1];
        obj->ctr[i + 2*XXTEA_NCOPIES] = in[2];
        obj->ctr[i + 3*XXTEA_NCOPIES] = in[3];
    }
    Xxtea128VecState_block(obj);
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


int scalar256_test(const CallerAPI *intf)
{
    static const uint32_t
        key[4]  = {0x08040201, 0x80402010, 0xf8fcfeff, 0x80c0e0f0},
        in[8]   = {0xc9f39adb, 0x0ca3366e, 0x976e3c64, 0x7a5bd7f4,
                   0x0ea4514b, 0xe559879d, 0x0bc4e381, 0x36441b34},
        ref[8]  = {0xe0b6f15e, 0x7b22a210, 0x4b3737a3, 0xc5ffbe59,
                   0x05033526, 0x51fb4547, 0x1e640030, 0x07d17d2c};
    Xxtea256State *obj = intf->malloc(sizeof(Xxtea256State));
    int is_ok = 1;
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key[i];
    }
    for (int i = 0; i < 8; i++) {
        obj->ctr[i] = in[i];
    }
    Xxtea256State_block(obj);
    is_ok = is_ok & cmp_vec4(intf, obj->out, ref);
    intf->free(obj);
    return is_ok;
}


int vector256_test(const CallerAPI *intf)
{
    static const uint32_t
        key[4]  = {0x08040201, 0x80402010, 0xf8fcfeff, 0x80c0e0f0},
        in[8]   = {0xc9f39adb, 0x0ca3366e, 0x976e3c64, 0x7a5bd7f4,
                   0x0ea4514b, 0xe559879d, 0x0bc4e381, 0x36441b34},
        ref[8]  = {0xe0b6f15e, 0x7b22a210, 0x4b3737a3, 0xc5ffbe59,
                   0x05033526, 0x51fb4547, 0x1e640030, 0x07d17d2c};
    int is_ok = 1;
    Xxtea256VecState *obj = intf->malloc(sizeof(Xxtea256VecState));
    Xxtea256VecState_init(obj, key);
    for (int i = 0; i < XXTEA_NCOPIES; i++) {
        for (int j = 0; j < 8; j++) {
            obj->ctr[i + j*XXTEA_NCOPIES] = in[j];
        }
    }
    Xxtea256VecState_block(obj);
    for (int i = 0; i < XXTEA_NCOPIES; i++) {
        intf->printf("COPY %2d: ", i);
        for (int j = 0; j < 8; j++) {
            uint32_t u = obj->out[i + XXTEA_NCOPIES*j];
            intf->printf("0x%.8X ", u);
            if (u != ref[j]) {
                is_ok = 0;
            }
        }
        intf->printf("\n");
    }
    intf->printf("REF:     ");
    for (int i = 0; i < 8; i++) {
        intf->printf("0x%.8X ", ref[i]);
    }
    intf->printf("\n");
    intf->free(obj);
    return is_ok;
}

/**
 * @details Test vectors are taken from the Crypt-XXTEA library.
 */
static int run_self_test(const CallerAPI *intf)
{
    int is_ok;
    intf->printf("----- Testing TEA with 128-bit block -----\n");
    is_ok = scalar_test(intf);
#ifdef XXTEA_VEC_ENABLED
    is_ok = is_ok & vector_test(intf);
#endif
    intf->printf("----- Testing TEA with 256-bit block -----\n");
    is_ok = is_ok & scalar256_test(intf);
#ifdef XXTEA_VEC_ENABLED
    is_ok = is_ok & vector256_test(intf);
#endif
    return is_ok;
}


static const char *get_prng_name(const CallerAPI *intf)
{
    return XxteaInfo_get(intf).name;
}

MAKE_UINT_PRNG("XXTEA", run_self_test, 32, get_prng_name)
