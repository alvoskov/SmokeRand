/**
 * @file hwtests.h
 * @brief Hamming weights bases tests implementation, mainly DC6.
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
    hamming_dc6_values, ///< Calculate Hamming weights for whole 32/64-bit values.
    hamming_dc6_bytes, ///< Process PRNG output as byte sequence.
    hamming_dc6_bytes_low8, ///< Make byte sequence from lower 8 bits of PRNG output
    hamming_dc6_bytes_low1 ///< Make byte sequence from lower 1 bit of PRNG output
} HammingDc6Mode;

/**
 * @brief Options for "DC6" test based on overlapping tuples
 * of specially encoded Hamming weights.
 */
typedef struct {
    unsigned long long nbytes; ///< Number of bytes processed by the test.
    HammingDc6Mode mode; ///< Selector of processed bits subset.
} HammingDc6Options;


typedef enum {
    hamming_dc6_w128 = 128,
    hamming_dc6_w256 = 256,
    hamming_dc6_w512 = 512
} HammingDc6WordSize;

/**
 * @brief Options for "DC6" test based on overlapping tuples
 * of specially encoded Hamming weights.
 */
typedef struct {
    unsigned long long nvalues; ///< Number of bytes processed by the test.
    HammingDc6WordSize wordsize; ///< Word size (128, 256 or 512 bits).
} HammingDc6LongOptions;


TestResults hamming_dc6_test(GeneratorState *obj, const HammingDc6Options *opts);
TestResults hamming_dc6_long_test(GeneratorState *obj, const HammingDc6LongOptions *opts);

#endif

