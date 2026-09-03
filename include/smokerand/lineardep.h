/**
 * @file lineardep.h
 * @brief Implementation of linear complexity and matrix rank tests.
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_LINEARDEP_H
#define __SMOKERAND_LINEARDEP_H
#include "smokerand/core.h"

/**
 * @brief Matrix rank test options.
 */
typedef struct {
    size_t n; ///< Size of nxn square matrix.
    unsigned int max_nbits; ///< Number of lower bits that will be used (8, 32, 64)
} MatrixRankOptions;


/**
 * @brief A special values of the `bitpos` field of the `LinearCompOptions`
 * structure. Needed for autoselection of lowest, highest and middle bits
 * without an explicit position.
 */
enum {
    LINEARCOMP_BITPOS_LOW  =  0, ///< Analyze the lowest bit.
    LINEARCOMP_BITPOS_HIGH = -1, ///< Analyze the middle bit.
    LINEARCOMP_BITPOS_MID  = -2  ///< Analyze the highest bit.
};

uint8_t *extract_bits_from_pos(GeneratorState *obj, size_t nbits, unsigned int bitpos);
size_t berlekamp_massey(const uint8_t *s, size_t n, uint8_t **out_poly);
long long linearcomp_L_to_T(size_t L, size_t nbits);

/**
 * @brief Linear complexity test options.
 * @details Note: bitpos field supports the special values
 * linearcomp_bitpos_low, linearcomp_bitpos_high, linearcomp_bitpos_mid
 */
typedef struct {
    size_t nbits; ///< Number of bits (recommended value is 200000)
    int bitpos; ///< Bit position (0 is the lowest).
} LinearCompOptions;

TestResults linearcomp_test(GeneratorState *obj, const LinearCompOptions *opts);
TestResults matrixrank_test(GeneratorState *obj, const MatrixRankOptions *opts);

// Unified interfaces that can be used for batteries composition
TestResults linearcomp_test_wrap(GeneratorState *obj, const void *udata);
TestResults matrixrank_test_wrap(GeneratorState *obj, const void *udata);

#endif // __SMOKERAND_LINEARDEP_H

