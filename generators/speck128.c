/**
 * @file speck128.c
 * @brief Speck128/128 CSPRNG cross-platform implementation for 64-bit
 * processors.
 * @details Contains three versions of Speck128/128:
 *
 * 1. Cross-platform scalar version (`--param=scalar`) that has period of
 *    \f$ 2^{129} \f$. Its performance is about 3.5 cpb on Intel(R) Core(TM)
 *    i5-11400H 2.70GHz. Also co
 * 2. Vectorized implementation based that uses AVX2 instruction set for modern
 *    x86-64 processors (`--param=vector-full`). Its period is \f$2^{64+5}\f$.
 *    Allows to achieve performance better than 1 cpb (about 0.75 cpb) on the
 *    same CPU. It is slightly faster than ChaCha12 and ISAAC64 CSPRNG.
 * 3. The version with reduced number of rounds, 16 insted of 32,
 *    `--param=vector-r16`, also uses AVX2 instructions. Its performance is
 *    about 0.35 cpb that is comparable to MWC or PCG generators.
 *
 * WARNING! The version with 16 rounds is not cryptographically secure!. However,
 * it is faster than the original Speck128/128 and probably is good enough to be
 * used as a general purpose PRNG. In [3] it is reported than 12 rounds is enough
 * to pass BigCrush and PractRand, this version uses 16.
 *
 * Periods of both `vector-full` and `vector-r16` versions is \f$ 2^{64 + 5} \f$:
 * they use 64-bit counters. The upper half of the block is used as a copy ID.
 *
 * References:
 *
 * 1. Ray Beaulieu, Douglas Shors et al. The SIMON and SPECK Families
 *    of Lightweight Block Ciphers // Cryptology ePrint Archive. 2013.
 *    Paper 2013/404. https://ia.cr/2013/404
 * 2. Ray Beaulieu, Douglas Shors et al. SIMON and SPECK implementation guide
 *    https://nsacyber.github.io/simon-speck/implementations/ImplementationGuide1.1.pdf
 * 3. Colin Josey. Reassessing the MCNP Random Number Generator. Technical
 *    Report LA-UR-23-25111. 2023. Los Alamos National Laboratory (LANL),
 *    Los Alamos, NM (United States) https://doi.org/10.2172/1998091
 *
 * Rounds of the `--param=scalar` version:
 *
 * - 8 rounds: passes `brief`, `default`, fails `full` (mainly `hamming_ot_long`)
 * - 9 rounds: passes `full` battery.
 *
 * - 8 rounds: passes SmallCrush, fails PractRand at 8 GiB
 * - 9 rounds: passes Crush and BigCrush, fails PractRand at ???
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#ifdef  __AVX2__
#define SPECK_VEC_ENABLED
#include "smokerand/x86exts.h"
#endif

PRNG_CMODULE_PROLOG

enum {
    NROUNDS_FULL = 32,
    NROUNDS_R16 = 16
};

/**
 * @brief Speck128 state.
 */
typedef struct {
    uint64_t ctr[2]; ///< Counter
    uint64_t out[2]; ///< Output buffer
    uint64_t keys[NROUNDS_FULL]; ///< Round keys
    unsigned int pos;
} Speck128State;


/**
 * @brief Speck128/128 state, vectorized version.
 * @details Counters vector (ctr) has the next layout:
 *
 *     [c0_lo, c1_lo, c2_lo, c3_lo; c0_hi, c1_hi, c2_hi, c3_hi;
 *      c4_lo, c5_lo, c6_lo, c7_lo; c4_hi, c5_hi, c6_hi, c7_hi]
 *
 * Output has the similar layout. It means that output of AVX version
 * is different from output of cross-platform 64-bit version
 */
typedef struct {
    uint64_t ctr[16]; ///< Counters
    uint64_t out[16]; ///< Output buffer
    uint64_t keys[NROUNDS_FULL]; ///< Round keys
    unsigned int pos; ///< Current position in the output buffer
    int nrounds;
} Speck128VecState;


/////////////////////////////////////////
///// Scalar version implementation /////
/////////////////////////////////////////

static inline void speck_round(uint64_t *x, uint64_t *y, const uint64_t k)
{
    *x = (rotr64(*x, 8) + *y) ^ k;
    *y = rotl64(*y, 3) ^ *x;
}


static void Speck128State_init(Speck128State *obj, const uint64_t *key)
{
    obj->ctr[0] = 0;
    obj->ctr[1] = 0;
    obj->keys[0] = key[0];
    obj->keys[1] = key[1];
    uint64_t a = obj->keys[0], b = obj->keys[1];
    for (size_t i = 0; i < NROUNDS_FULL - 1; i++) {
        speck_round(&b, &a, i);
        obj->keys[i + 1] = a;
    }    
    obj->pos = 2;
}

static inline void Speck128State_block(Speck128State *obj)
{
    obj->out[0] = obj->ctr[0];
    obj->out[1] = obj->ctr[1];
    for (size_t i = 0; i < NROUNDS_FULL; i++) {
        speck_round(&(obj->out[1]), &(obj->out[0]), obj->keys[i]);
    }
}

static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    Speck128State *obj = intf->malloc(sizeof(Speck128State));
    uint64_t key[2];
    key[0] = intf->get_seed64();
    key[1] = intf->get_seed64();
    Speck128State_init(obj, key);
    (void) gi;
    return obj;
}


/**
 * @brief Speck128/128 implementation.
 */
static inline uint64_t get_bits_scalar_raw(void *state)
{
    Speck128State *obj = state;
    if (obj->pos == 2) {
        Speck128State_block(obj);
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(scalar)

/**
 * @brief Internal self-test based on test vectors.
 */
int run_self_test_scalar(const CallerAPI *intf)
{
    const uint64_t key[] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    const uint64_t ctr[] = {0x7469206564616d20, 0x6c61766975716520};
    const uint64_t out[] = {0x7860fedf5c570d18, 0xa65d985179783265};
    Speck128State *obj = intf->malloc(sizeof(Speck128State));
    Speck128State_init(obj, key);
    obj->ctr[0] = ctr[0]; obj->ctr[1] = ctr[1];
    Speck128State_block(obj);
    intf->printf("Output:    0x%16llX 0x%16llX\n", obj->out[0], obj->out[1]);
    intf->printf("Reference: 0x%16llX 0x%16llX\n", out[0], out[1]);
    int is_ok = obj->out[0] == out[0] && obj->out[1] == out[1];
    intf->free(obj);
    return is_ok;
}

/////////////////////////////////////////////
///// Vectorized version implementation /////
/////////////////////////////////////////////

#ifdef SPECK_VEC_ENABLED
/**
 * @brief Vectorized round function for encryption procedure. Processes
 * 4 copies of Speck128/128 simultaneously.
 */
static inline void round_avx(__m256i *x, __m256i *y, __m256i *kv)
{
    *x = mm256_rotr_epi64_def(*x, 8);
    *x = _mm256_add_epi64(*x, *y);
    *x = _mm256_xor_si256(*x, *kv);
    *y = mm256_rotl_epi64_def(*y, 3);
    *y = _mm256_xor_si256(*y, *x);
}
#endif


/**
 * @brief Initialize counters, buffers and key schedule.
 * @param obj Pointer to the object to be initialized.
 * @param key Pointer to 128-bit key. If it is NULL then random key will be
 * automatically generated.
 */
void Speck128VecState_init(Speck128VecState *obj, const uint64_t *key, int nrounds)
{
    // Initialize counters
    // a) Generators 0..3
    obj->ctr[0] = 0; obj->ctr[4] = 0;
    obj->ctr[1] = 1; obj->ctr[5] = 1;
    obj->ctr[2] = 2; obj->ctr[6] = 2;
    obj->ctr[3] = 3; obj->ctr[7] = 4;
    // b) Generators 4..7
    obj->ctr[8] = 4; obj->ctr[12] = 8;
    obj->ctr[9] = 5; obj->ctr[13] = 16;
    obj->ctr[10] = 6; obj->ctr[14] = 32;
    obj->ctr[11] = 7; obj->ctr[15] = 64;
    // Initialize key schedule
    obj->keys[0] = key[0];
    obj->keys[1] = key[1];
    uint64_t a = obj->keys[0], b = obj->keys[1];
    for (size_t i = 0; i < NROUNDS_FULL - 1; i++) {
        speck_round(&b, &a, i);
        obj->keys[i + 1] = a;
    }
    obj->nrounds = nrounds;
    // Initialize output buffers
    obj->pos = 16;
}


/**
 * @brief Generate block of 1024 pseudorandom bits.
 */
static inline void Speck128VecState_block(Speck128VecState *obj)
{
#ifdef SPECK_VEC_ENABLED
    __m256i a = _mm256_loadu_si256((__m256i *) (void *) obj->ctr);
    __m256i b = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 4));
    __m256i c = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 8));
    __m256i d = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 12));
    for (int i = 0; i < obj->nrounds; i++) {
        __m256i kv = _mm256_set1_epi64x((long long) obj->keys[i]);
        round_avx(&b, &a, &kv);
        round_avx(&d, &c, &kv);
    }
    _mm256_storeu_si256((__m256i *) (void *) obj->out, a);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 4), b);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 8), c);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 12), d);
#else
    (void) obj;
#endif
}

/**
 * @brief Increase counters of all 8 copies of CSPRNG. The 64-bit counters
 * are used.
 */
static inline void Speck128VecState_inc_counter(Speck128VecState *obj)
{
#ifdef SPECK_VEC_ENABLED
    const __m256i inc = _mm256_set1_epi64x(1);
    __m256i ctr0 = _mm256_loadu_si256((__m256i *) (void *) &obj->ctr[0]); // 0-3
    __m256i ctr8 = _mm256_loadu_si256((__m256i *) (void *) &obj->ctr[8]); // 8-11
    ctr0 = _mm256_add_epi64(ctr0, inc);
    ctr8 = _mm256_add_epi64(ctr8, inc);
    _mm256_storeu_si256((__m256i *) (void *) &obj->ctr[0], ctr0);
    _mm256_storeu_si256((__m256i *) (void *) &obj->ctr[8], ctr8);
#else
    (void) obj;
#endif
}


static void *create_vector(const CallerAPI *intf, int nrounds)
{
#ifdef SPECK_VEC_ENABLED
    Speck128VecState *obj = intf->malloc(sizeof(Speck128VecState));
    uint64_t key[2];
    key[0] = intf->get_seed64();
    key[1] = intf->get_seed64();
    Speck128VecState_init(obj, key, nrounds);
    return obj;
#else
    (void) intf; (void) nrounds;
    return NULL;
#endif
}


static void *create_vector_full(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_vector(intf, NROUNDS_FULL);
}

static void *create_vector_reduced(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_vector(intf, NROUNDS_R16);
}


/**
 * @brief Get 64-bit value from Speck128/128.
 */
static inline uint64_t get_bits_vector_raw(void *state)
{
    Speck128VecState *obj = state;
    if (obj->pos == 16) {
        Speck128VecState_block(obj);
        Speck128VecState_inc_counter(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(vector)

/**
 * @brief Internal self-test based on test vectors for a full 32-round version.
 */
int run_self_test_vector_full(const CallerAPI *intf)
{
#ifdef SPECK_VEC_ENABLED
    const uint64_t key[] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    const uint64_t ctr[] = {0x7469206564616d20, 0x6c61766975716520};
    const uint64_t out[] = {0x7860fedf5c570d18, 0xa65d985179783265};
    int is_ok = 1;
    Speck128VecState *obj = intf->malloc(sizeof(Speck128VecState));
    Speck128VecState_init(obj, key, NROUNDS_FULL);
    for (size_t i = 0; i < 4; i++) {
        obj->ctr[i] = ctr[0]; obj->ctr[i + 8] = ctr[0];
        obj->ctr[i + 4] = ctr[1]; obj->ctr[i + 12] = ctr[1];
    }
    Speck128VecState_block(obj);
    intf->printf("%16s %16s\n", "Output", "Reference");
    for (size_t i = 0; i < 16; i++) {
        size_t ind = i / 4;
        if (ind > 1) ind -= 2;
        intf->printf("0x%16llX 0x%16llX\n", obj->out[i], out[ind]);
        if (obj->out[i] != out[ind]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
#else
    intf->printf("vector-full version is not supported: AVX2 is required\n");
    return 1;
#endif
}


/**
 * @brief Internal self-test based on test vectors for a simplified 16-round
 * version (essentially a scrambler, not cipher).
 * @details These vectors are taken from the original Speck128/128 with
 * 32 rounds. But block encryption procedure is called two times with
 * additional code for updating round keys, copying output to counters etc.
 */
int run_self_test_vector_reduced(const CallerAPI *intf)
{
#ifdef SPECK_VEC_ENABLED
    const uint64_t key[] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    const uint64_t ctr[] = {0x7469206564616d20, 0x6c61766975716520};
    const uint64_t out[] = {0x7860fedf5c570d18, 0xa65d985179783265};
    int is_ok = 1;
    Speck128VecState *obj = intf->malloc(sizeof(Speck128VecState));
    Speck128VecState_init(obj, key, NROUNDS_R16);
    for (unsigned int i = 0; i < 4; i++) {
        obj->ctr[i] = ctr[0]; obj->ctr[i + 8] = ctr[0];
        obj->ctr[i + 4] = ctr[1]; obj->ctr[i + 12] = ctr[1];
    }
    // Rounds 0..15
    Speck128VecState_block(obj);
    // Rounds 16..32
    for (unsigned int i = 0; i < 16; i++) {
        obj->ctr[i] = obj->out[i];
    }
    uint64_t a = key[0], b = key[1];
    for (unsigned int i = 0; i < 2*NROUNDS_R16 - 1; i++) {
        speck_round(&b, &a, i);
        if (i >= 15) {
            obj->keys[i - 15] = a;
        }
    }
    // Rounds 16..31
    Speck128VecState_block(obj);
    // Print results
    intf->printf("%16s %16s\n", "Output", "Reference");
    for (size_t i = 0; i < 16; i++) {
        size_t ind = i / 4;
        if (ind > 1) ind -= 2;
        intf->printf("0x%16llX 0x%16llX\n", obj->out[i], out[ind]);
        if (obj->out[i] != out[ind]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
#else
    intf->printf("vector-r16 version is not supported: AVX2 is required\n");    
    return 1;
#endif
}

//////////////////////
///// Interfaces /////
//////////////////////


static inline uint64_t get_bits_raw(void *state)
{
    (void) state;
    return 0;
}

static inline void *create(const CallerAPI *intf)
{
    (void) intf;
    return NULL;
}

int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    (void) get_bits_raw(NULL);
    intf->printf("----- Speck128/128: 32 rounds (scalar) -----\n");
    is_ok = is_ok & run_self_test_scalar(intf);
    intf->printf("----- Speck128/128: 32 rounds (vectorized) -----\n");
    is_ok = is_ok & run_self_test_vector_full(intf);
    intf->printf("----- Speck128/128: 16 rounds (vectorized) -----\n");
    is_ok = is_ok & run_self_test_vector_reduced(intf);
    return is_ok;
}



static const char description[] =
"Speck128/128 block cipher based PRNGs\n"
"param values are supported:\n"
"  full - scalar portable version with the full number of rounds (default)\n"
"  vector-full - AVX2 version with the full number of rounds\n"
"  vector-r16  - AVX2 version with the halved (reduced) number of rounds\n"
"Only 'full' versions are cryptographically secure. However the version with\n"
"16 rounds passes empirical tests for randomness.\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 64;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "full") || !intf->strcmp(param, "")) {
        gi->name = "Speck128:full";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "vector-full")) {
        gi->name = "Speck128:vector-full";
        gi->create = create_vector_full;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else if (!intf->strcmp(param, "vector-r16")) {
        gi->name = "Speck128:vector-r16";
        gi->create = create_vector_reduced;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else {
        gi->name = "Speck128:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
