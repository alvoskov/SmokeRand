/**
 * @file threefry2x64.c
 * @brief An implementation of Threefry2x64x20 PRNGs. It is a simplified
 * version of Threefish with reduced size of blocks and rounds.
 *
 * @details Differences from Threefish:
 *
 * 1. Reduced number of rounds (20 instead of 72).
 * 2. Reduced block size: only 128 bit.
 * 3. Counter is used as a text.
 * 4. No XORing in output generation.
 *
 * References:
 *
 * 1. J. K. Salmon, M. A. Moraes, R. O. Dror and D. E. Shaw, "Parallel random
 *    numbers: As easy as 1, 2, 3," SC '11: Proceedings of 2011 International
 *    Conference for High Performance Computing, Networking, Storage and
 *    Analysis, Seattle, WA, USA, 2011, pp. 1-12.
 *    https://doi.org/10.1145/2063384.2063405.
 * 2. Random123: a Library of Counter-Based Random Number Generators
 *    https://github.com/girving/random123/blob/main/tests/kat_vectors
 * 3. https://www.schneier.com/academic/skein/threefish/
 * 4. https://pdebuyl.be/blog/2016/threefry-rng.html
 *
 * @copyright Threefish block cipher was developed by Bruce Schneier et al.
 * Threefry algorithm was proposed by J. K. Salmon, M. A. Moraes, R. O. Dror
 * and D. E. Shaw.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#ifdef  __AVX2__
#define TF128_VEC_ENABLED
#include "smokerand/x86exts.h"
#endif

PRNG_CMODULE_PROLOG


static const int rot2x64[8] = {16, 42, 12, 31, 16, 32, 24, 21};

/**
 * @brief Number of 64-bit words per state of one copy of ThreeFry.
 * DON'T CHANGE!
 */
#define NWORDS 2

/**
 * @brief Number of ThreeFry copies per vectorized version.
 * @details It can be set to 4, 8 or 16. Measurements show that
 * 8 and 16 are slightly faster than 4. Performance of 8 and 16
 * is almost identical, so 8 was selected as a default value to
 * consume less memory per PNG state.
 */
#define NCOPIES 8

/**
 * @brief Number of 256-bit AVX registers required to save x0 or x1
 * vectors (upper or lower halves of PRNG state).
 */
#define NREGS (NCOPIES / 4)



/**
 * @brief Threefry 2x64x20 scalar version state.
 */
typedef struct {
    uint64_t k[NWORDS + 1]; ///< Key schedule
    uint64_t p[NWORDS]; ///< Counter ("plain text")
    uint64_t v[NWORDS]; ///< Output buffer
    size_t pos;
} Threefry2x64State;


/**
 * @brief Threefry 2x64x20 vectorized (AVX2) version state.
 * @details The PRNG state is vectorized and uses the next layout
 * for the `ctr` (counter) vector:
 *
 *     [x0(0) x0(1) ... x0(NCOPIES-1) x1(0) x1(1) x1(NCOPIES-1)]
 */
typedef struct {
    uint64_t k[NWORDS + 1]; ///< Key schedule
    uint64_t ctr[NCOPIES * NWORDS]; ///< Vectorized counters
    uint64_t out[NCOPIES * NWORDS]; ///< Vectorized output buffer
    size_t pos;
} Threefry2x64AVXState;


/////////////////////////////////////////////////////////
///// Threefry2x64x20 scalar version implementation /////
/////////////////////////////////////////////////////////


static void Threefry2x64State_init(Threefry2x64State *obj, const uint64_t *k)
{
    static const uint64_t C240 = 0x1BD11BDAA9FC1A22ULL;
    obj->k[NWORDS] = C240;
    for (size_t i = 0; i < NWORDS; i++) {
        obj->k[i] = k[i];
        obj->p[i] = 0;
        obj->k[NWORDS] ^= obj->k[i];
    }
}


static inline void mix2(uint64_t *x, int d)
{
    x[0] += x[1];
    x[1] = rotl64(x[1], d);
    x[1] ^= x[0];
}

static inline void inject_key(uint64_t *out, const uint64_t *ks,
    size_t n, size_t i0, size_t i1)
{    
    out[0] += ks[i0];
    out[1] += ks[i1] + n;
}

#define MIX2_ROT_0_3 \
    mix2(v, rot2x64[0]); mix2(v, rot2x64[1]); \
    mix2(v, rot2x64[2]); mix2(v, rot2x64[3]);

#define MIX2_ROT_4_7 \
    mix2(v, rot2x64[4]); mix2(v, rot2x64[5]); \
    mix2(v, rot2x64[6]); mix2(v, rot2x64[7]);


EXPORT void Threefry2x64State_block20(Threefry2x64State *obj)
{
    uint64_t v[NWORDS];
    for (size_t i = 0; i < NWORDS; i++) {
        v[i] = obj->p[i];
    }
    // Initial key injection
    inject_key(v, obj->k, 0,  0, 1);
    // Rounds 0-3
    MIX2_ROT_0_3; inject_key(v, obj->k, 1,  1, 2);
    // Rounds 4-7
    MIX2_ROT_4_7; inject_key(v, obj->k, 2,  2, 0);
    // Rounds 8-11
    MIX2_ROT_0_3; inject_key(v, obj->k, 3,  0, 1);
    // Rounds 12-15
    MIX2_ROT_4_7; inject_key(v, obj->k, 4,  1, 2);
    // Rounds 16-19
    MIX2_ROT_0_3; inject_key(v, obj->k, 5,  2, 0);
    // Output generation
    for (size_t i = 0; i < NWORDS; i++) {
        obj->v[i] = v[i];
    }
}



static inline void Threefry2x64State_inc_counter(Threefry2x64State *obj)
{
    obj->p[0]++;
    /* obj->p[1] is reserved for thread number */
}


/**
 * @brief Comparison of vectors for internal self-tests.
 */
static int self_test_compare(const CallerAPI *intf,
    const uint64_t *out, const uint64_t *ref)
{
    intf->printf("OUT: ");
    int is_ok = 1;
    for (size_t i = 0; i < NWORDS; i++) {
        intf->printf("%llX ", out[i]);
        if (out[i] != ref[i])
            is_ok = 0;
    }
    intf->printf("\n");
    intf->printf("REF: ");
    for (size_t i = 0; i < NWORDS; i++) {
        intf->printf("%llX ", ref[i]);
    }
    intf->printf("\n");
    return is_ok;
}

/**
 * @brief An internal self-test. Test vectors are taken
 * from Random123 library.
 */
static int run_self_test_scalar(const CallerAPI *intf)
{
    Threefry2x64State obj;
    static const uint64_t k0_m1[4] = {-1, -1}, ctr_m1[4] = {-1, -1};
    static const uint64_t ref20_m1[4] = {0xe02cb7c4d95d277aull, 0xd06633d0893b8b68ull};

    static const uint64_t ctr_pi[4] = {0x243f6a8885a308d3ull, 0x13198a2e03707344ull};
    static const uint64_t k0_pi[4] = {0xa4093822299f31d0ull, 0x082efa98ec4e6c89ull};
    static const uint64_t ref20_pi[4] = {0x263c7d30bb0f0af1ull, 0x56be8361d3311526ull};

    intf->printf("----- Self-test for the scalar version -----\n");
    intf->printf("Threefry2x64x20 ('-1' example)\n");
    Threefry2x64State_init(&obj, k0_m1);
    obj.p[0] = ctr_m1[0]; obj.p[1] = ctr_m1[1];
    Threefry2x64State_block20(&obj);
    if (!self_test_compare(intf, obj.v, ref20_m1)) {
        return 0;
    }

    intf->printf("Threefry2x64x20 ('pi' example)\n");
    Threefry2x64State_init(&obj, k0_pi);
    obj.p[0] = ctr_pi[0]; obj.p[1] = ctr_pi[1];
    Threefry2x64State_block20(&obj);
    if (!self_test_compare(intf, obj.v, ref20_pi)) {
        return 0;
    }
    return 1;
}


/////////////////////////////////////////////////////////////
///// Threefry2x64x20 vectorized version implementation /////
/////////////////////////////////////////////////////////////



/**
 * @brief Initializes the PRNG state: fills the key schedule
 * and resets counters.
 */
void Threefry2x64AVXState_init(Threefry2x64AVXState *obj, const uint64_t *k)
{
    static const uint64_t C240 = 0x1BD11BDAA9FC1A22ULL;
    obj->k[NWORDS] = C240;
    for (size_t i = 0; i < NWORDS; i++) {
        obj->k[i] = k[i];
        obj->k[NWORDS] ^= obj->k[i];
    }
    for (size_t i = 0; i < NCOPIES; i++) {
        obj->ctr[i] = 0;
        obj->ctr[i + NCOPIES] = i;
    }
    obj->pos = NWORDS * NCOPIES;
}

#ifdef TF128_VEC_ENABLED
/**
 * @brief Vectorized round function of Threefry2x64 (very similar to Threefish).
 */
static inline void mix2v(__m256i *x0v, __m256i *x1v, int d)
{
    for (int i = 0; i < NREGS; i++) {
        x0v[i] = _mm256_add_epi64(x0v[i], x1v[i]); // x[0] += x[1];
        x1v[i] = mm256_rotl_epi64_def(x1v[i], d); // x[1] = rotl64(x[1], d);
        x1v[i] = _mm256_xor_si256(x1v[i], x0v[i]); // x[1] ^= x[0];
    }
}


static inline void inject_key_vec(__m256i *x0v, __m256i *x1v, const uint64_t *ks,
    size_t n, size_t i0, size_t i1)
{
    __m256i ks0 = _mm256_set1_epi64x(ks[i0]);
    __m256i ks1 = _mm256_set1_epi64x(ks[i1] + n);
    for (int i = 0; i < NREGS; i++) {
        x0v[i] = _mm256_add_epi64(x0v[i], ks0);
        x1v[i] = _mm256_add_epi64(x1v[i], ks1);
    }
}

#define MIX2VEC_ROT_0_3(n, i0, i1) \
    mix2v(x0v, x1v, rot2x64[0]); mix2v(x0v, x1v, rot2x64[1]); \
    mix2v(x0v, x1v, rot2x64[2]); mix2v(x0v, x1v, rot2x64[3]); \
    inject_key_vec(x0v, x1v, k, n,  i0, i1);

#define MIX2VEC_ROT_4_7(n, i0, i1) \
    mix2v(x0v, x1v, rot2x64[4]); mix2v(x0v, x1v, rot2x64[5]); \
    mix2v(x0v, x1v, rot2x64[6]); mix2v(x0v, x1v, rot2x64[7]); \
    inject_key_vec(x0v, x1v, k, n,  i0, i1);


static inline void make_block_vec(__m256i *x0v, __m256i *x1v, const uint64_t *k)
{
    // Initial key injection
    inject_key_vec(x0v, x1v, k, 0,  0, 1);
    MIX2VEC_ROT_0_3(1,  1, 2); // Rounds 0-3
    MIX2VEC_ROT_4_7(2,  2, 0); // Rounds 4-7
    MIX2VEC_ROT_0_3(3,  0, 1); // Rounds 8-11
    MIX2VEC_ROT_4_7(4,  1, 2); // Rounds 12-15
    MIX2VEC_ROT_0_3(5,  2, 0); // Rounds 16-19
}

#endif // TF128_VEC_ENABLED

EXPORT void Threefry2x64AVXState_block20(Threefry2x64AVXState *obj)
{
#ifdef TF128_VEC_ENABLED
    __m256i x0v[NREGS], x1v[NREGS];
#if NCOPIES == 4
    x0v[0] = _mm256_loadu_si256((__m256i *) (void *) obj->ctr);
    x1v[0] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 4));
#elif NCOPIES == 8
    x0v[0] = _mm256_loadu_si256((__m256i *) (void *) obj->ctr);
    x0v[1] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 4));
    x1v[0] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 8));
    x1v[1] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 12));
#elif NCOPIES == 16
    x0v[0] = _mm256_loadu_si256((__m256i *) (void *) obj->ctr);
    x0v[1] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 4));
    x0v[2] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 8));
    x0v[3] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 12));
    x1v[0] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 16));
    x1v[1] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 20));
    x1v[2] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 24));
    x1v[3] = _mm256_loadu_si256((__m256i *) (void *) (obj->ctr + 28));
#else
#error Unknown number of PRNG copies
#endif
    make_block_vec(x0v, x1v, obj->k);

#if NCOPIES == 4
    _mm256_storeu_si256((__m256i *) (void *) obj->out, x0v[0]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 4), x1v[0]);
#elif NCOPIES == 8
    _mm256_storeu_si256((__m256i *) (void *) obj->out, x0v[0]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 4), x0v[1]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 8), x1v[0]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 12), x1v[1]);
#elif NCOPIES == 16
    _mm256_storeu_si256((__m256i *) (void *) obj->out, x0v[0]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 4), x0v[1]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 8), x0v[2]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 12), x0v[3]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 16), x1v[0]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 20), x1v[1]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 24), x1v[2]);
    _mm256_storeu_si256((__m256i *) (void *) (obj->out + 28), x1v[3]);
#else
#error Unknown number of PRNG copies
#endif
#else // TF128_VEC_ENABLED
    (void) obj;
#endif
}



static inline void Threefry2x64AVXState_inc_counter(Threefry2x64AVXState *obj)
{
#ifdef TF128_VEC_ENABLED
    for (int i = 0; i < NCOPIES; i++) {
        obj->ctr[i]++;
    }
#else
    (void) obj;
#endif
}



/**
 * @brief Comparison of vectors for internal self-tests.
 */
int self_test_compare_vector(const CallerAPI *intf,
    const uint64_t *out, const uint64_t *ref)
{
    intf->printf("OUT: ");
    int is_ok = 1;
    for (size_t i = 0; i < NWORDS * NCOPIES; i++) {
        intf->printf("%llX ", out[i]);
        if (out[i] != ref[i / NCOPIES])
            is_ok = 0;
        if ((i + 1) % NWORDS == 0) {
            intf->printf("\n");
        }
    }
    intf->printf("\n");
    intf->printf("REF: ");
    for (size_t i = 0; i < NWORDS * NCOPIES; i++) {
        intf->printf("%llX ", ref[i / NCOPIES]);
        if ((i + 1) % NWORDS == 0) {
            intf->printf("\n");
        }
    }
    intf->printf("\n");
    return is_ok;
}

/**
 * @brief An internal self-test. Test vectors are taken from Random123 library.
 */
static int run_self_test_vector(const CallerAPI *intf)
{
#ifdef TF128_VEC_ENABLED
    Threefry2x64AVXState obj;
    static const uint64_t k0_m1[4] = {-1, -1}, ctr_m1[4] = {-1, -1},
        ref20_m1[4] = {0xe02cb7c4d95d277aull, 0xd06633d0893b8b68ull},
        ctr_pi[4]   = {0x243f6a8885a308d3ull, 0x13198a2e03707344ull},
        k0_pi[4]    = {0xa4093822299f31d0ull, 0x082efa98ec4e6c89ull},
        ref20_pi[4] = {0x263c7d30bb0f0af1ull, 0x56be8361d3311526ull};

    intf->printf("----- Self-test for the vectorized version -----\n");
    intf->printf("Threefry2x64x20 ('-1' example)\n");
    Threefry2x64AVXState_init(&obj, k0_m1);
    for (int i = 0; i < NCOPIES; i++) {
        obj.ctr[i] = ctr_m1[0]; obj.ctr[i + NCOPIES] = ctr_m1[1];
    }
    Threefry2x64AVXState_block20(&obj);
    if (!self_test_compare_vector(intf, obj.out, ref20_m1)) {
        return 0;
    }

    intf->printf("Threefry2x64x20 ('pi' example)\n");
    Threefry2x64AVXState_init(&obj, k0_pi);
    for (int i = 0; i < NCOPIES; i++) {
        obj.ctr[i] = ctr_pi[0]; obj.ctr[i + NCOPIES] = ctr_pi[1];
    }
    Threefry2x64AVXState_block20(&obj);
    if (!self_test_compare_vector(intf, obj.out, ref20_pi)) {
        return 0;
    }
#else
    intf->printf("----- Vectorized (AVX2) implementation is not available -----\n");
#endif
    return 1;
}



/////////////////////////////////////
///// Module external interface /////
/////////////////////////////////////


static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    is_ok = is_ok & run_self_test_scalar(intf);
    is_ok = is_ok & run_self_test_vector(intf);
    return is_ok;
}


static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    uint64_t k[NWORDS];
    Threefry2x64State *obj = intf->malloc(sizeof(Threefry2x64State));
    for (int i = 0; i < NWORDS; i++) {
        k[i] = intf->get_seed64();
    }
    Threefry2x64State_init(obj, k);
    (void) gi;
    return obj;
}


static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef TF128_VEC_ENABLED
    uint64_t k[NWORDS];
    Threefry2x64AVXState *obj = intf->malloc(sizeof(Threefry2x64AVXState));
    for (int i = 0; i < NWORDS; i++) {
        k[i] = intf->get_seed64();
    }
    Threefry2x64AVXState_init(obj, k);
    (void) gi;
    return obj;
#else
    (void) gi;
    intf->printf("AVX2 is not available on this platform\n");
    return NULL;
#endif
}

static void *create(const CallerAPI *intf)
{
    (void) intf;
    return NULL;
}

static inline uint64_t get_bits_scalar_raw(void *state)
{
    Threefry2x64State *obj = state;
    if (obj->pos >= NWORDS) {
        Threefry2x64State_inc_counter(obj);
        Threefry2x64State_block20(obj);
        obj->pos = 0;
    }
    return obj->v[obj->pos++];
}

static inline uint64_t get_bits_vector_raw(void *state)
{
    Threefry2x64AVXState *obj = state;
    if (obj->pos >= NCOPIES * NWORDS) {
        Threefry2x64AVXState_inc_counter(obj);
        Threefry2x64AVXState_block20(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

MAKE_GET_BITS_WRAPPERS(scalar)
MAKE_GET_BITS_WRAPPERS(vector)



int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = NULL;
    gi->nbits = 64;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "scalar") || !intf->strcmp(param, "")) {
        gi->name = "Threefry2x64x20:scalar";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "avx2")) {
        gi->name = "Threefry2x64x20:AVX2";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
    } else {
        gi->name = "Threefry2x64x20:unknown";
        gi->create = default_create;
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
