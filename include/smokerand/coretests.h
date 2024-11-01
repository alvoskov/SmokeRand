/**
 * @file coretests.h
 * @brief The key tests for testings PRNGs: frequency tests, Marsaglia's
 * birthday spacings and monkey tests, Knuth's gap test.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_CORETESTS_H
#define __SMOKERAND_CORETESTS_H
#include "smokerand/core.h"
/**
 * @brief Options for n-dimensional birthday spacings tests.
 */
typedef struct {
    unsigned int nbits_per_dim; ///< Number of bits per dimension.
    unsigned int ndims; ///< Number of dimensions.
    unsigned int nsamples; ///< Number of samples.
    int get_lower; ///< 0/1 - use lower/higher part of PRNG output.
} BSpaceNDOptions;

/**
 * @brief Options for gap test.
 * @details Recommended options:
 *
 * - shl = 9, ngaps = 1e7
 * - shl = 10, ngaps = 1e7
 * - shl = 10, ngaps = 1e8
 */
typedef struct {
    unsigned int shl; ///< Gap is [0; 2^{-shl})
    unsigned long ngaps; ///< Number of gaps
} GapOptions;

/**
 * @brief Sets which bits of random number will be analysed.
 */
typedef enum {
    use_bits_all,
    use_bits_low8,
    use_bits_low1
} UseBitsMode;

/**
 * @brief Options for "DC6" test based on overlapping tuples
 * of specially encoded Hamming weights.
 */
typedef struct {
    unsigned long long nsamples; ///< Number of processed pseudorandom numbers.
    UseBitsMode use_bits; ///< Selector of processed bits subset.
} HammingDc6Options;

TestResults bspace_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts);
TestResults collisionover_test(GeneratorState *obj, const BSpaceNDOptions *opts);
TestResults gap_test(GeneratorState *obj, const GapOptions *opts);
TestResults monobit_freq_test(GeneratorState *obj);
TestResults byte_freq_test(GeneratorState *obj);
TestResults word16_freq_test(GeneratorState *obj);
TestResults hamming_dc6_test(GeneratorState *obj, const HammingDc6Options *opts);


#endif
