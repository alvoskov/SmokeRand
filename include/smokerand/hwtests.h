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
    use_bits_all,
    use_bits_low8,
    use_bits_low1
} UseBitsMode;

/**
 * @brief Modes of Hamming weights based DC6 test. Each mode corresponds
 * to the bit chunks that will be used for computation of Hamming weights.
 */
typedef enum {
    hamming_ot_values, ///< Calculate Hamming weights for whole 32/64-bit values.
    hamming_ot_bytes, ///< Process PRNG output as byte sequence.
    hamming_ot_bytes_low8, ///< Make byte sequence from lower 8 bits of PRNG output
    hamming_ot_bytes_low1 ///< Make byte sequence from lower 1 bit of PRNG output
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
    hamming_ot_w128 = 128,
    hamming_ot_w256 = 256,
    hamming_ot_w512 = 512,
    hamming_ot_w1024 = 1024
} HammingOtWordSize;

/**
 * @brief Options for "DC6" test based on overlapping tuples
 * of specially encoded Hamming weights.
 */
typedef struct {
    unsigned long long nvalues; ///< Number of bytes processed by the test.
    HammingOtWordSize wordsize; ///< Word size (128, 256 or 512 bits).
} HammingOtLongOptions;


TestResults hamming_ot_test(GeneratorState *obj, const HammingOtOptions *opts);
TestResults hamming_ot_long_test(GeneratorState *obj, const HammingOtLongOptions *opts);

#endif

