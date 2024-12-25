/**
 * @file speck128_avx_shared.c
 * @brief Speck128/128 CSPRNG vectorized implementation for AVX2
 * instruction set for modern x86-64 processors. Its period is \f$ 2^{129} \f$.
 * Allows to achieve performance better than 1 cpb (about 0.74 cpb) on
 * Intel(R) Core(TM) i5-11400H 2.70GHz. It is slightly faster than ChaCha12
 * and ISAAC64 CSPRNG.
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

#define NROUNDS 32

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
    *x = (rotr64(*x, 8) + *y) ^ k;
    *y = rotl64(*y, 3) ^ *x;
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
    obj->ctr[1] = 1; obj->ctr[5] = 0;
    obj->ctr[2] = 2; obj->ctr[6] = 0;
    obj->ctr[3] = 3; obj->ctr[7] = 0;
    // b) Generators 4..7
    obj->ctr[8] = 4; obj->ctr[12] = 0;
    obj->ctr[9] = 5; obj->ctr[13] = 0;
    obj->ctr[10] = 6; obj->ctr[14] = 0;
    obj->ctr[11] = 7; obj->ctr[15] = 0;
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
    if ((obj->ctr[0] += 8) == 0) obj->ctr[4] += 8;
    if ((obj->ctr[1] += 8) == 0) obj->ctr[5] += 8;
    if ((obj->ctr[2] += 8) == 0) obj->ctr[6] += 8;
    if ((obj->ctr[3] += 8) == 0) obj->ctr[7] += 8;
    if ((obj->ctr[8] += 8) == 0) obj->ctr[12] += 8;
    if ((obj->ctr[9] += 8) == 0) obj->ctr[13] += 8;
    if ((obj->ctr[10] += 8) == 0) obj->ctr[14] += 8;
    if ((obj->ctr[11] += 8) == 0) obj->ctr[15] += 8;
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
 */
int run_self_test(const CallerAPI *intf)
{
    const uint64_t key[] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    const uint64_t ctr[] = {0x7469206564616d20, 0x6c61766975716520};
    const uint64_t out[] = {0x7860fedf5c570d18, 0xa65d985179783265};
    int is_ok = 1;
    Speck128VecState *obj = intf->malloc(sizeof(Speck128VecState));
    Speck128VecState_init(obj, key, intf);
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
}


MAKE_UINT64_PRNG("Speck128AVX", run_self_test)
