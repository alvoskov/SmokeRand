/**
 * @file speck128_r16_avx_shared.c
 * @brief Speck128/128 version with reduced (halved) number of rounds and
 * simplified 64-bit counters. It is optimized for AVX2 instructions set for
 * modern x86-64 processors. Its period is \f$ 2^{68} \f$. Allows to achieve
 * performance better than 1 cpb (about 0.35 cpb) on Intel(R) Core(TM) i5-11400H
 * 2.70GHz. It is comparable to MWC or PCG generators.
 *
 * WARNING! It is not CSPRNG. However, it is faster than the original
 * Speck128/128 and probably is good enough to be used as a general purpose PRNG.
 * In [3] it is reported than 12 rounds is enough to pass BigCrush and PractRand,
 * this version uses 16.
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
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
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

#define NROUNDS 16

static inline uint64_t rotl(uint64_t x, uint64_t r)
{
    return (x << r) | (x >> (64 - r));
}

static inline uint64_t rotr(uint64_t x, uint64_t l)
{
    return (x << (64 - l)) | (x >> l);
}

/**
 * @brief Vectorized "rotate left" instruction for vector of 64-bit values.
 */
static inline __m256i mm256_rotl_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi64(in, r), _mm256_srli_epi64(in, 64 - r));
}

/**
 * @brief Vectorized "rotate right" instruction for vector of 64-bit values.
 */
static inline __m256i mm256_rotr_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi64(in, 64 - r), _mm256_srli_epi64(in, r));
}

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
    uint64_t keys[NROUNDS]; ///< Round keys
    unsigned int pos; ///< Current position in the output buffer
} Speck128VecState;

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

/**
 * @brief Round function for key schedule generation (scalar version).
 */
static inline void round_scalar(uint64_t *x, uint64_t *y, const uint64_t k)
{
    *x = rotr(*x, 8);
    *x += *y;
    *x ^= k;
    *y = rotl(*y, 3);
    *y ^= *x;
}

/**
 * @brief Initialize counters, buffers and key schedule.
 * @param obj Pointer to the object to be initialized.
 * @param key Pointer to 128-bit key. If it is NULL then random key will be
 * automatically generated.
 */
static void Speck128VecState_init(Speck128VecState *obj, const uint64_t *key, const CallerAPI *intf)
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
    if (key == NULL) {
        obj->keys[0] = intf->get_seed64();
        obj->keys[1] = intf->get_seed64();
    } else {
        obj->keys[0] = key[0];
        obj->keys[1] = key[1];
    }
    uint64_t a = obj->keys[0], b = obj->keys[1];
    for (size_t i = 0; i < NROUNDS - 1; i++) {
        round_scalar(&b, &a, i);
        obj->keys[i + 1] = a;
    }
    // Initialize output buffers
    obj->pos = 16;
}


/**
 * @brief Generate block of 1024 pseudorandom bits.
 */
static inline void Speck128VecState_block(Speck128VecState *obj)
{
    __m256i a = _mm256_loadu_si256((__m256i *) obj->ctr);
    __m256i b = _mm256_loadu_si256((__m256i *) (obj->ctr + 4));
    __m256i c = _mm256_loadu_si256((__m256i *) (obj->ctr + 8));
    __m256i d = _mm256_loadu_si256((__m256i *) (obj->ctr + 12));
    for (size_t i = 0; i < NROUNDS; i++) {
        __m256i kv = _mm256_set1_epi64x(obj->keys[i]);
        round_avx(&b, &a, &kv);
        round_avx(&d, &c, &kv);
    }
    _mm256_storeu_si256((__m256i *) obj->out, a);
    _mm256_storeu_si256((__m256i *) (obj->out + 4), b);
    _mm256_storeu_si256((__m256i *) (obj->out + 8), c);
    _mm256_storeu_si256((__m256i *) (obj->out + 12), d);
}

/**
 * @brief Increase counters of all 8 copies of CSPRNG.
 */
static inline void Speck128VecState_inc_counter(Speck128VecState *obj)
{
    const __m256i inc = _mm256_set1_epi64x(1);
    __m256i ctr0 = _mm256_loadu_si256((__m256i *) &obj->ctr[0]); // 0-3
    __m256i ctr8 = _mm256_loadu_si256((__m256i *) &obj->ctr[8]); // 8-11
    ctr0 = _mm256_add_epi64(ctr0, inc);
    ctr8 = _mm256_add_epi64(ctr8, inc);
    _mm256_storeu_si256((__m256i *) &obj->ctr[0], ctr0);
    _mm256_storeu_si256((__m256i *) &obj->ctr[8], ctr8);
}


static void *create(const CallerAPI *intf)
{
    Speck128VecState *obj = intf->malloc(sizeof(Speck128VecState));
    Speck128VecState_init(obj, NULL, intf);
    return (void *) obj;
}


/**
 * @brief Get 64-bit value from Speck128/128.
 */
static inline uint64_t get_bits_raw(void *state)
{
    Speck128VecState *obj = state;
    if (obj->pos == 16) {
        Speck128VecState_block(obj);
        Speck128VecState_inc_counter(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

/**
 * @brief Internal self-test based on test vectors.
 * @details These vectors are taken from the original Speck128/128 with
 * 32 rounds. But block encryption procedure is called two times with
 * additional code for updating round keys, copying output to counters etc.
 */
int run_self_test(const CallerAPI *intf)
{
    const uint64_t key[] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    const uint64_t ctr[] = {0x7469206564616d20, 0x6c61766975716520};
    const uint64_t out[] = {0x7860fedf5c570d18, 0xa65d985179783265};
    int is_ok = 1;
    Speck128VecState *obj = intf->malloc(sizeof(Speck128VecState));
    Speck128VecState_init(obj, key, intf);
    for (int i = 0; i < 4; i++) {
        obj->ctr[i] = ctr[0]; obj->ctr[i + 8] = ctr[0];
        obj->ctr[i + 4] = ctr[1]; obj->ctr[i + 12] = ctr[1];
    }
    // Rounds 0..15
    Speck128VecState_block(obj);
    // Rounds 16..32
    for (int i = 0; i < 16; i++) {
        obj->ctr[i] = obj->out[i];
    }
    uint64_t a = key[0], b = key[1];
    for (int i = 0; i < 2*NROUNDS - 1; i++) {
        round_scalar(&b, &a, i);
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
}


MAKE_UINT64_PRNG("Speck128AVX", run_self_test)
