/**
 * @file lineardep.c
 * @brief Implementation of linear complexity and matrix rank tests.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/lineardep.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>


#ifdef __AVX2__
#pragma message "AVX2 version will be compiled"
#include <x86intrin.h>
#define VECINT_NBITS 256
typedef __m256i VECINT;
static inline void xorbits(VECINT *a_j, const VECINT *a_i, size_t i1, size_t i2)
{
    for (size_t k = i1; k < i2; k++) {
        __m256i aj_k = _mm256_loadu_si256(a_j + k);
        __m256i ai_k = _mm256_loadu_si256(a_i + k);
        aj_k = _mm256_xor_si256(aj_k, ai_k);
        _mm256_storeu_si256(a_j + k, aj_k);
    }
}
#else
#pragma message "X64-64 version will be compiled"
#define VECINT_NBITS 64
typedef uint64_t VECINT;
static inline void xorbits(VECINT *a_j, const VECINT *a_i, size_t i1, size_t i2)
{
    for (size_t k = i1; k < i2; k++)
        a_j[k] ^= a_i[k];
}
#endif

#define GET_AJI (row_ptr[j][i_offset] & i_mask)
#define SWAP_ROWS(i, j) { uint32_t *ptr = row_ptr[i]; \
    row_ptr[i] = row_ptr[j]; row_ptr[j] = ptr; }


/**
 * @brief Calculate rank of binary matrix.
 */
static size_t calc_bin_matrix_rank(uint32_t *a, size_t n)
{
    uint32_t **row_ptr = calloc(n, sizeof(uint32_t *));
    // Initialize some intermediate buffers
    for (size_t i = 0; i < n; i++) {
        row_ptr[i] = a + i * n / 32;
    }
    // Diagonalize the matrix
    size_t rank = 0;
    for (size_t i = 0; i < n; i++) {
        // Some useful offsets
        size_t i_offset = i / 32, i_mask = 1 << (i % 32);
        size_t i_offset_64 = i / VECINT_NBITS;
        // Find the j-th row where a(j,i) is not zero, swap it
        // with i-th row and make Gaussian elimination
        size_t j = rank;
        while (j < n && GET_AJI == 0) { j++; } // a_ji == 0
        if (j < n) {
            SWAP_ROWS(i, j)
            VECINT *a_i = (VECINT *) row_ptr[i];
            for (size_t j = i + 1; j < n; j++) {
                VECINT *a_j = (VECINT *) row_ptr[j];
                if (GET_AJI != 0) {
                    xorbits(a_j, a_i, i_offset_64, n / VECINT_NBITS);
                }
            }
            rank++;
        }
    }
    free(row_ptr);
    return rank;
}


/**
 * @brief Matrix rank tests based on generation of random matrices.
 * @details References:
 * 
 * 1. Rukhin A., Soto J. et al. A Statistical Test Suite for Random and
 *    Pseudorandom Number Generators for Cryptographic Applications //
 *    NIST SP 800-22 Rev. 1a. https://doi.org/10.6028/NIST.SP.800-22r1a
 * 2. Kolchin V. F. Random graphs. Cambridge University Press. 1998.
 *    ISBN 9780511721342
 *    https://doi.org/10.1017/CBO9780511721342
 * 3. OEIS A048651 (https://oeis.org/A048651).
 *
 * @param n Size of nxn square matrix.
 * @param max_nbits Number of lower bits that will be used (8, 32, 64)
 */
TestResults matrixrank_test(GeneratorState *obj, size_t n, unsigned int max_nbits)
{
    int nmat = 64, Oi[3] = {0, 0, 0};
    double pi[3] = {0.1284, 0.5776, 0.2888};
    size_t mat_len = n * n / 32;
    size_t min_rank = n + 1;
    TestResults ans = {.name = "mrank", .x = 0, .p = NAN};
    uint32_t *a = calloc(mat_len, sizeof(uint32_t));
    obj->intf->printf("Matrix rank test\n");
    obj->intf->printf("  n = %d. Number of matrices: %d\n", (int) n, nmat);
    for (int i = 0; i < nmat; i++) {
        size_t mat_len = n * n / 32;
        // Prepare nthreads matrices for threads
        if (max_nbits == 8) {
            uint8_t *a8 = (uint8_t *) a;
            for (size_t j = 0; j < mat_len * 4; j++) {
                a8[j] = obj->gi->get_bits(obj->state) & 0xFF;
            }
        } else if (obj->gi->nbits == 32) {
            for (size_t j = 0; j < mat_len; j++) {
                a[j] = obj->gi->get_bits(obj->state);
            }
        } else {
            uint64_t *a64 = (uint64_t *) a;
            for (size_t j = 0; j < mat_len / 2; j++) {
                a64[j] = obj->gi->get_bits(obj->state);
            }
        }
        // Calculate matrix rank
        size_t rank = calc_bin_matrix_rank(a, n);
        if (rank >= n - 2) {
            Oi[rank - (n - 2)]++;
        } else {
            Oi[0]++;
        }
        if (rank < min_rank) {
            min_rank = rank;
        }
    }
    free(a);
    // Computation of p-value
    obj->intf->printf("  %5s %10s %10s\n", "rank", "Oi", "Ei");
    for (int i = 0; i < 3; i++) {
        double Ei = pi[i] * nmat;
        ans.x += pow(Oi[i] - Ei, 2.0) / Ei;
        obj->intf->printf("  %5d %10d %10.4g\n", i + (int) n - 2, Oi[i], Ei);
    }
    ans.p = exp(-0.5 * ans.x);
    ans.alpha = -expm1(-0.5 * ans.x);
    obj->intf->printf("  Minimal observed rank: %lu\n", (unsigned long) min_rank);
    obj->intf->printf("  x = %g; p = %g; 1-p = %g\n", ans.x, ans.p, ans.alpha);
    obj->intf->printf("\n");
    return ans;
}


/**
 * @brief Performs the a[i] ^= b[i] operation for the given vectors.
 * @param a    This vector (a) will be modified.
 * @param b    This vector (b) will not be modified.
 * @param len  Vectors lengths.
 */
static inline void xorbytes(uint8_t *a, const uint8_t *b, size_t len)
{
/*
    // Simpler but non optimized code
    for (size_t i = 0; i < len; i++) {
        a[i] ^= b[i];
    }
*/    
    uint64_t *av = (uint64_t *) a, *bv = (uint64_t *) b;
    size_t nwords = len / 8;
    for (size_t i = 0; i < nwords; i++) {
        av[i] ^= bv[i];
    }
    a += nwords * 8;
    b += nwords * 8;
    for (size_t i = 0; i < len % 8; i++) {
        a[i] ^= b[i];
    }    
}

/**
 * @brief Berlekamp-Massey algorithm for computation of linear complexity
 * of bit sequence.
 * @param s Bit sequence, each byte corresponds to ONE bit.
 * @param n Number of bits (length of s array).
 * @return Linear complexity.
 */
size_t berlekamp_massey(const uint8_t *s, size_t n)
{
    size_t L = 0; // Complexity
    size_t N = 0; // Current position
    long m = -1;
    uint8_t *C = calloc(n, sizeof(uint8_t)); C[0] = 1; // Coeffs.
    uint8_t *B = calloc(n, sizeof(uint8_t)); B[0] = 1; // Prev.coeffs.
    uint8_t *T = calloc(n, sizeof(uint8_t)); // Temp. copy of coeffs.
    while (N < n) {
        char d = s[N];
        for (size_t i = 1; i <= L; i++) {
            d ^= C[i] & s[N - i];
        }
        if (d == 1) {
            memcpy(T, C, (L + 1) * sizeof(uint8_t));
            xorbytes(&C[N - m], B, n - N + m);
            if (2*L <= N) {
                L = N + 1 - L;
                m = N;
                memcpy(B, T, (L + 1) * sizeof(uint8_t));
            }
        }
        N++;
    }
    free(C);
    free(B);
    free(T);
    return L;
}



/**
 * @brief Linear complexity test based on Berlekamp-Massey algorithm.
 * @details Formula for p-value computation may be found in:
 *
 * 1. M.Z. Wang. Linear complexity profiles and jump complexity //
 *    // Information Processing Letters. 1997. V. 61. P. 165-168.
 *    https://doi.org/10.1016/S0020-0190(97)00004-5
 *
 * @param nbits Number of bits (recommended value is 200000)
 * @param bitpos Bit position (0 is the lowest).
 */
TestResults linearcomp_test(GeneratorState *obj, size_t nbits, unsigned int bitpos)
{
    TestResults ans = {.name = "linearcomp", .x = NAN, .p = NAN};
    uint8_t *s = calloc(nbits, sizeof(uint8_t));
    obj->intf->printf("Linear complexity test\n");
    obj->intf->printf("  nbits: %lld\n", (long long) nbits);
    uint64_t mask = 1ull << bitpos;
    for (size_t i = 0; i < nbits; i++) {
        if (obj->gi->get_bits(obj->state) & mask)
            s[i] = 1;
    }
    double parity = nbits & 1;
    double mu = nbits / 2.0 + (9.0 - parity) / 36.0;
    double sigma = sqrt(86.0/81.0);
    ans.x = berlekamp_massey(s, nbits);
    double z = (ans.x - mu) / sigma;
    ans.p = 0.5 * erfc(-z / sqrt(2.0));
    ans.alpha = 0.5 + 0.5 * erf(-z / sqrt(2.0));
    obj->intf->printf("  L = %g; z = %g; p = %g\n", ans.x, z, ans.p);
    obj->intf->printf("\n");
    return ans;
}
