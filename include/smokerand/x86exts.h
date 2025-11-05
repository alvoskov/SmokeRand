/**
 * @file x86exts.h
 * @brief Cross-compiler header file for usage of x86 specific intrinstics.
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_X86EXTS_H
#define __SMOKERAND_X86EXTS_H

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


#ifdef __AVX2__
/**
 * @brief Vectorized "rotate left" instruction for vector of 64-bit values.
 */
static inline __m256i mm256_rotl_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(
        _mm256_slli_epi64(in, r),
        _mm256_srli_epi64(in, 64 - r)
    );
}

/**
 * @brief Vectorized "rotate right" instruction for vector of 64-bit values.
 */
static inline __m256i mm256_rotr_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(
        _mm256_slli_epi64(in, 64 - r),
        _mm256_srli_epi64(in, r)
    );
}
#endif // __AVX2__

#endif // __SMOKERAND_X86EXTS_H
