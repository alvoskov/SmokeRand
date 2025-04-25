/**
 * @file hwtests.h
 * @brief Hamming weights bases tests implementation, mainly based
 * on overlapping tuples and similar to DC6 from PractRand.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_HWTESTS_H
#define __SMOKERAND_HWTESTS_H
#include "smokerand/core.h"

/**
 * @brief Sets which bits of random number will be analysed.
 */
typedef enum {
    USE_BITS_ALL,
    USE_BITS_LOW8,
    USE_BITS_LOW1
} UseBitsMode;

/**
 * @brief Modes of Hamming weights based DC6 test. Each mode corresponds
 * to the bit chunks that will be used for computation of Hamming weights.
 */
typedef enum {
    HAMMING_OT_VALUES, ///< Calculate Hamming weights for whole 32/64-bit values.
    HAMMING_OT_BYTES, ///< Process PRNG output as byte sequence.
    HAMMING_OT_BYTES_LOW8, ///< Make byte sequence from lower 8 bits of PRNG output
    HAMMING_OT_BYTES_LOW1 ///< Make byte sequence from lower 1 bit of PRNG output
} HammingOtMode;

/**
 * @brief Options for "DC6" test based on overlapping tuples
 * of specially encoded Hamming weights.
 */
typedef struct {
    unsigned long long nbytes; ///< Number of bytes processed by the test.
    HammingOtMode mode; ///< Selector of processed bits subset.
} HammingOtOptions;


typedef enum {
    HAMMING_OT_W128 = 128,
    HAMMING_OT_W256 = 256,
    HAMMING_OT_W512 = 512,
    HAMMING_OT_W1024 = 1024
} HammingOtWordSize;

/**
 * @brief Options for "DC6" test based on overlapping tuples
 * of specially encoded Hamming weights.
 */
typedef struct {
    unsigned long long nvalues; ///< Number of bytes processed by the test.
    HammingOtWordSize wordsize; ///< Word size (128, 256 or 512 bits).
} HammingOtLongOptions;


typedef struct {
    unsigned long long nvalues; ///< Number of pseudorandom values processed by the test.
} HammingDistrOptions;


TestResults hamming_ot_test(GeneratorState *obj, const HammingOtOptions *opts);
TestResults hamming_ot_long_test(GeneratorState *obj, const HammingOtLongOptions *opts);
TestResults hamming_distr_test(GeneratorState *obj, const HammingDistrOptions *opts);

// Unified interfaces that can be used for batteries composition
TestResults hamming_ot_test_wrap(GeneratorState *obj, const void *udata);
TestResults hamming_ot_long_test_wrap(GeneratorState *obj, const void *udata);
TestResults hamming_distr_test_wrap(GeneratorState *obj, const void *udata);

#endif

