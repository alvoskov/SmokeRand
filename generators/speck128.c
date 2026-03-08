/**
 * @file speck128.c
 * @brief Speck128/128 stream cipher based PRNG.
 * @details Contains three versions of Speck128/128:
 *
 * 1. Cross-platform scalar version (`--param=scalar`) that has period of
 *    \f$ 2^{129} \f$. Its performance is about 3.5 cpb on Intel(R) Core(TM)
 *    i5-11400H 2.70GHz.
 * 2. Vectorized implementation based that uses AVX2 instruction set for modern
 *    x86-64 processors (`--param=vector-full`). Its period is \f$2^{64+5}\f$.
 *    Allows to achieve performance better than 1 cpb (about 0.75 cpb) on the
 *    same CPU. It is slightly faster than ChaCha12 and ISAAC64 CSPRNG.
 * 3. The version with reduced number of rounds, 16 insted of 32,
 *    `--param=vector-r16`, also uses AVX2 instructions. Its performance is
 *    about 0.35 cpb that is comparable to MWC or PCG generators.
 *
 * WARNING! This program is designed as a general purpose high quality PRNG
 * for simulations and statistical testing. IT IS NOT DESIGNED FOR ENCRYPTION,
 * KEYS/NONCES GENERATION AND OTHER CRYPTOGRAPHICAL APPLICATION!
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
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
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
    NROUNDS_R16 = 16,
    NCOPIES = 8,
    NCOPIES_POW2 = 3 // 2^3 = 8
};

// Uncomment it to make AVX2 version of Speck128/128 5-10% faster if its
// output can be different from the scalar version of Speck128/128
//
// #define ALLOW_VECTOR_SHUFFLE

/**
 * @brief Speck128 state.
 */
typedef struct {
    uint64_t ctr[2]; ///< Counter
    uint64_t out[2]; ///< Output buffer
    uint64_t keys[NROUNDS_FULL]; ///< Round keys
    unsigned int pos;
    int nrounds;
} Speck128State;


/**
 * @brief Speck128/128 state, vectorized version.
 * @details Counters vector (ctr) has the next layout:
 *
 * New:
 *     [c0-c7_lo; c0-c7_hi]; lo - for counters, hi - for threads ids
 *
 * Old:
 *     [c0_lo, c1_lo, c2_lo, c3_lo; c0_hi, c1_hi, c2_hi, c3_hi;
 *      c4_lo, c5_lo, c6_lo, c7_lo; c4_hi, c5_hi, c6_hi, c7_hi]
 *
 * Output has the similar layout. It means that output of AVX version
 * is different from output of cross-platform 64-bit version
 */
typedef struct {
    uint64_t ctr[2 * NCOPIES]; ///< Counters
    uint64_t out[2 * NCOPIES]; ///< Output buffer
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


static inline void Speck128State_block(Speck128State *obj)
{
    obj->out[0] = obj->ctr[0];
    obj->out[1] = obj->ctr[1];
    for (int i = 0; i < obj->nrounds; i++) {
        speck_round(&(obj->out[1]), &(obj->out[0]), obj->keys[i]);
    }
}


static void Speck128State_init(Speck128State *obj, const uint64_t *key, int nrounds)
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
    obj->nrounds = nrounds;
    obj->pos = 0;
    Speck128State_block(obj);
}


static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf, int nrounds)
{
    Speck128State *obj = intf->malloc(sizeof(Speck128State));
    uint64_t key[2];
    key[0] = intf->get_seed64();
    key[1] = intf->get_seed64();
    Speck128State_init(obj, key, nrounds);
    (void) gi;
    return obj;
}


static void *create_scalar_full(const GeneratorInfo *gi, const CallerAPI *intf)
{
    return create_scalar(gi, intf, NROUNDS_FULL);
}


static void *create_scalar_reduced(const GeneratorInfo *gi, const CallerAPI *intf)
{
    return create_scalar(gi, intf, NROUNDS_R16);
}


/**
 * @brief Speck128/128 implementation.
 */
static inline uint64_t get_bits_scalar_raw(void *state)
{
    Speck128State *obj = state;
    if (obj->pos == 2) {
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        Speck128State_block(obj);
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
    // Test for full round version 
    Speck128State_init(obj, key, NROUNDS_FULL);
    obj->ctr[0] = ctr[0]; obj->ctr[1] = ctr[1];
    Speck128State_block(obj);
    intf->printf("Full round scalar version\n");
    intf->printf("Output:    0x%16llX 0x%16llX\n",
        (unsigned long long) obj->out[0], (unsigned long long) obj->out[1]);
    intf->printf("Reference: 0x%16llX 0x%16llX\n",
        (unsigned long long) out[0], (unsigned long long) out[1]);
    int is_ok = obj->out[0] == out[0] && obj->out[1] == out[1];
    // Test for reduced round version
    Speck128State_init(obj, key, NROUNDS_R16);
    obj->ctr[0] = ctr[0]; obj->ctr[1] = ctr[1];
    // Rounds 0..15
    Speck128State_block(obj);
    // Rounds 16..32
    obj->ctr[0] = obj->out[0]; obj->ctr[1] = obj->out[1];
    uint64_t a = key[0], b = key[1];
    for (unsigned int i = 0; i < 2*NROUNDS_R16 - 1; i++) {
        speck_round(&b, &a, i);
        if (i >= 15) {
            obj->keys[i - 15] = a;
        }
    }
    Speck128State_block(obj);
    intf->printf("Reduced round scalar version\n");
    intf->printf("Output:    0x%16llX 0x%16llX\n",
        (unsigned long long) obj->out[0], (unsigned long long) obj->out[1]);
    intf->printf("Reference: 0x%16llX 0x%16llX\n",
        (unsigned long long) out[0], (unsigned long long) out[1]);
    is_ok = is_ok && obj->out[0] == out[0] && obj->out[1] == out[1];

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
 * @brief Generate block of 1024 pseudorandom bits.
 */
static inline void Speck128VecState_block(Speck128VecState *obj)
{
#ifdef SPECK_VEC_ENABLED
    __m256i hi[NCOPIES / 4], lo[NCOPIES / 4];
    for (int i = 0; i < NCOPIES / 4; i++) {
        hi[i] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 4*i));
        lo[i] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + NCOPIES + 4*i));
    }
    for (int i = 0; i < obj->nrounds; i++) {
        __m256i kv = _mm256_set1_epi64x((long long) obj->keys[i]);
        for (int j = 0; j < NCOPIES / 4; j++) {
            round_avx(&lo[j], &hi[j], &kv);
        }
    }
    for (int i = 0; i < NCOPIES / 4; i++) {
        _mm256_storeu_si256((__m256i *) (void *) (obj->out + 4*i), hi[i]);
        _mm256_storeu_si256((__m256i *) (void *) (obj->out + NCOPIES + 4*i), lo[i]);
    }
#else
    (void) obj;
#endif
}


/**
 * @brief Initialize counters, buffers and key schedule.
 * @param obj Pointer to the object to be initialized.
 * @param key Pointer to 128-bit key. If it is NULL then random key will be
 * automatically generated.
 */
void Speck128VecState_init(Speck128VecState *obj, const uint64_t *key, int nrounds)
{
    // Initialize counters in the upper half of the block
    for (unsigned int i = 0; i < NCOPIES; i++) {
        obj->ctr[i] = i;
        obj->ctr[i + NCOPIES] = 0;
    }
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
    Speck128VecState_block(obj);
    obj->pos = 0;
}


/**
 * @brief Increase counters of all 8 copies of CSPRNG. The 64-bit counters
 * are used.
 */
static inline void Speck128VecState_inc_counter(Speck128VecState *obj)
{
#ifdef SPECK_VEC_ENABLED
    const __m256i inc = _mm256_set1_epi64x(NCOPIES);
    for (int i = 0; i < NCOPIES; i += 4) {
        __m256i ctr = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + i));
        ctr = _mm256_add_epi64(ctr, inc);
       _mm256_storeu_si256((__m256i *) (void *) (obj->ctr + i), ctr);
    }
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
    if (obj->pos == 2 * NCOPIES) {
        Speck128VecState_inc_counter(obj);
        Speck128VecState_block(obj);
        obj->pos = 0;
    }
    // lo0 lo1 lo2 .. lo7
    // hi1 hi2 hi3 .. hi7
#ifdef ALLOW_VECTOR_SHUFFLE
    return obj->out[obj->pos++];
#else
    const size_t i = obj->pos >> 1, j = obj->pos & 0x1;
    obj->pos++;
    return obj->out[i + (j << NCOPIES_POW2)];
#endif
}

MAKE_GET_BITS_WRAPPERS(vector)


#ifdef SPECK_VEC_ENABLED
static int run_self_test_vector_nrounds(const CallerAPI *intf, int nrounds)
{
    const uint64_t key[] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    int is_ok = 1;
    Speck128State *obj_ref = intf->malloc(sizeof(Speck128State));
    Speck128VecState *obj = intf->malloc(sizeof(Speck128VecState));
    Speck128State_init(obj_ref, key, nrounds);
    Speck128VecState_init(obj, key, nrounds);
    for (long i = 0; i < 10000000; i++) {
        const uint64_t u_ref = get_bits_scalar_raw(obj_ref);
        const uint64_t u = get_bits_vector_raw(obj);
        if (i < 32) {
            intf->printf("%2d) Out=0x%16llX Ref=0x%16llX\n", i,
                (unsigned long long) u, (unsigned long long) u_ref);
        }
        if (u != u_ref) {
            is_ok = 0;
        }
    }
    intf->free(obj_ref);
    intf->free(obj);
    return is_ok;
}
#endif

/**
 * @brief Internal self-test based on test vectors for a full 32-round version.
 */
int run_self_test_vector(const CallerAPI *intf)
{
#ifdef SPECK_VEC_ENABLED
    intf->printf("vector-full test\n");
    int is_ok = run_self_test_vector_nrounds(intf, NROUNDS_FULL);
    intf->printf("vector-r16 test\n");
    is_ok = is_ok && run_self_test_vector_nrounds(intf, NROUNDS_R16);
    return is_ok;
#else
    intf->printf("vector-full version is not supported: AVX2 is required\n");
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
    intf->printf("----- Speck128/128: scalar version -----\n");
    is_ok = is_ok & run_self_test_scalar(intf);
    intf->printf("----- Speck128/128: vectorized version -----\n");
    is_ok = is_ok & run_self_test_vector(intf);
    return is_ok;
}


static const char description[] =
"Speck128/128 block cipher based PRNGs\n"
"param values are supported:\n"
"  full        - scalar portable version with the full number of rounds\n"
"                (default)\n"
"  reduced     - scalar portable version with the halved (reduced) number\n"
"                of rounds\n"
"  vector-full - AVX2 version with the full number of rounds\n"
"  vector-r16  - AVX2 version with the halved (reduced) number of rounds\n"
"Only 'full' versions correspond to the original Speck128/128. However\n"
"the weakened 16 rounds versions pass empirical tests for randomness.\n";


/**
 * @brief Speck128/128 versions description.
 */
static const GeneratorParamVariant gen_list[] = {
    {"",            "Speck128:full",        64, create_scalar_full,    get_bits_scalar, get_sum_scalar},
    {"full",        "Speck128:full",        64, create_scalar_full,    get_bits_scalar, get_sum_scalar},
    {"reduced",     "Speck128:r16",         64, create_scalar_reduced, get_bits_scalar, get_sum_scalar},
#ifdef SPECK_VEC_ENABLED
    {"vector-full", "Speck128:vector-full", 64, create_vector_full,    get_bits_vector, get_sum_vector},
    {"vector-r16",  "Speck128:vector-r16",  64, create_vector_reduced, get_bits_vector, get_sum_vector},
#endif
    GENERATOR_PARAM_VARIANT_EMPTY
};


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
