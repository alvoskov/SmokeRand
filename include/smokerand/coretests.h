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
    unsigned long nsamples; ///< Number of samples.
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
 * @brief Settings for frequencies of n-bit words test.
 * @details Recommended settings:
 *
 * - for bytes: {8, 256, 4096}
 * - for 16-bit words: {16, 16, 4096}
 */
typedef struct {
    unsigned int bits_per_word; ///< Bits per word
    unsigned int average_freq; ///< Average frequency for a bin
    size_t nblocks; ///< Number of blocks for K.-S. criterion
} NBitWordsFreqOptions;

TestResults bspace_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts);
TestResults bspace64_1d_ns_test(GeneratorState *obj, unsigned int nsamples);
TestResults bspace4_8d_decimated_test(GeneratorState *obj, unsigned int step);
TestResults collisionover_test(GeneratorState *obj, const BSpaceNDOptions *opts);
TestResults gap_test(GeneratorState *obj, const GapOptions *opts);
TestResults gap16_count0_test(GeneratorState *obj, long long ngaps);
TestResults sumcollector_test(GeneratorState *obj, unsigned long long nvalues);
TestResults mod3_test(GeneratorState *obj, unsigned long long nvalues);
TestResults monobit_freq_test(GeneratorState *obj);
TestResults nbit_words_freq_test(GeneratorState *obj,
    const NBitWordsFreqOptions *opts);
TestResults byte_freq_test(GeneratorState *obj);
TestResults word16_freq_test(GeneratorState *obj);


#endif
