/**
 * @file lineardep.c
 * @brief Implementation of linear complexity and matrix rank tests.
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/lineardep.h"
#include "smokerand/specfuncs.h"
#ifdef __AVX2__
    #include "smokerand/x86exts.h"
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>


static inline void xorbits(uint32_t *a_j, const uint32_t *a_i, size_t i1, size_t i2)
{
#ifdef __AVX2__
    #pragma message ("AVX2 version will be compiled")
    i1 >>= 3; i2 >>= 3; // From 32-bit to 256-bit chunks
    __m256i *a_j_vec = (__m256i *) (void *) a_j;
    __m256i *a_i_vec = (__m256i *) (void *) a_i;
    for (size_t k = i1; k < i2; k++) {
        __m256i aj_k = _mm256_loadu_si256(a_j_vec + k);
        __m256i ai_k = _mm256_loadu_si256(a_i_vec + k);
        aj_k = _mm256_xor_si256(aj_k, ai_k);
        _mm256_storeu_si256(a_j_vec + k, aj_k);
    }
#else
    #pragma message ("Portable version will be compiled")
    for (size_t k = i1; k < i2; k++)
        a_j[k] ^= a_i[k];
#endif
}


#define GET_AJI (row_ptr[j][i_offset] & i_mask)

static inline void swap_rows(uint32_t **row_ptr, size_t i, size_t j)
{
    uint32_t *ptr = row_ptr[i];
    row_ptr[i] = row_ptr[j];
    row_ptr[j] = ptr;
}

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
        const size_t i_offset = i / 32;
        //const size_t i_offset_block = i / VECINT_NBITS;
        const uint32_t i_mask = (uint32_t)1U << (i % 32);
        // Find the j-th row where a(j,i) is not zero, swap it
        // with i-th row and make Gaussian elimination
        size_t j = rank;
        while (j < n && GET_AJI == 0) { j++; } // a_ji == 0
        if (j < n) {
            swap_rows(row_ptr, i, j);
            uint32_t *a_i = row_ptr[i];
            for (j = i + 1; j < n; j++) {
                uint32_t *a_j = row_ptr[j];
                if (GET_AJI != 0) {
                    xorbits(a_j, a_i, i_offset/*_block*/, n / 32 /* / VECINT_NBITS*/);
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
 * @param opts Test options (matrix size, number of lower bits from each value)
 */
TestResults matrixrank_test(GeneratorState *obj, const MatrixRankOptions *opts)
{
    TestResults ans = TestResults_create("mrank");
    int nmat = 64, Oi[3] = {0, 0, 0};
    const double pi[3] = {0.1284, 0.5776, 0.2888};
    size_t n = opts->n;
    //unsigned int max_nbits = opts->max_nbits;
    const size_t mat_len = n * n / 32;
    size_t min_rank = n + 1;
    uint32_t *a = calloc(mat_len, sizeof(uint32_t));
    obj->intf->printf("Matrix rank test\n");
    obj->intf->printf("  n = %d. Number of matrices: %d; max_nbits: %u\n",
        (int) n, nmat, opts->max_nbits);
    for (int i = 0; i < nmat; i++) {
        if (opts->max_nbits == 8) {
            for (size_t j = 0; j < mat_len; j++) {
                const uint32_t u0 = (uint32_t) obj->gi->get_bits(obj->state) & 0xFF;
                const uint32_t u1 = (uint32_t) obj->gi->get_bits(obj->state) & 0xFF;
                const uint32_t u2 = (uint32_t) obj->gi->get_bits(obj->state) & 0xFF;
                const uint32_t u3 = (uint32_t) obj->gi->get_bits(obj->state) & 0xFF;
                a[j] = u0 | (u1 << 8) | (u2 << 16) | (u3 << 24);
            }
        } else if (obj->gi->nbits == 32) {
            for (size_t j = 0; j < mat_len; j++) {
                a[j] = (uint32_t) obj->gi->get_bits(obj->state);
            }
        } else if (obj->gi->nbits == 64) {
            for (size_t j = 0, pos = 0; j < mat_len / 2; j++) {
                const uint64_t u = obj->gi->get_bits(obj->state);
                a[pos++] = (uint32_t) (u & 0xFFFFFFFF);
                a[pos++] = (uint32_t) (u >> 32);
            }
        } else {
            obj->intf->printf(
                "Matrix rank is undefined for %d-bit PRNG and %d-bit chunks\n",
                (int) obj->gi->nbits,
                (int) opts->max_nbits
            );
            return ans;
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
    ans.x = 0.0;
    for (int i = 0; i < 3; i++) {
        double Ei = pi[i] * nmat;
        ans.x += pow(Oi[i] - Ei, 2.0) / Ei;
        obj->intf->printf("  %5d %10d %10.4g\n", i + (int) n - 2, Oi[i], Ei);
    }
    if (opts->max_nbits <= 8) {
        ans.penalty = PENALTY_MATRIXRANK_LOW;
    } else {
        ans.penalty = PENALTY_MATRIXRANK;
    }
    ans.p = sr_chi2_pvalue(ans.x, 2); // exp(-0.5 * ans.x);
    ans.alpha = sr_chi2_cdf(ans.x, 2); // -expm1(-0.5 * ans.x);
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
    for (size_t i = 0; i < len; i++) {
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
    uint8_t *C = calloc(n, sizeof(uint8_t)); // Coeffs.
    uint8_t *B = calloc(n, sizeof(uint8_t)); // Prev.coeffs.
    uint8_t *T = calloc(n, sizeof(uint8_t)); // Temp. copy of coeffs.
    if (C == NULL || B == NULL || T == NULL) {
        fprintf(stderr, "***** berlekamp_massey: not enough memory *****");
        free(C); free(B); free(T);
        exit(EXIT_FAILURE);
    }
    C[0] = 1; B[0] = 1;
    while (N < n) {
        uint8_t d = s[N];
        for (size_t i = 1; i <= L; i++) {
            d ^= C[i] & s[N - i];
        }
        if (d == 1) {
            memcpy(T, C, (L + 1) * sizeof(uint8_t));
            xorbytes(&C[(long) N - m], B, (size_t) ((long) n - (long) N + m));
            if (2*L <= N) {
                L = N + 1 - L;
                m = (long) N;
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
 * @brief Recalculate special bitpos values (relative offsets) to absolute ones.
 */
static unsigned int linearcomp_get_bitpos(const GeneratorState *obj, const LinearCompOptions *opts)
{
    if (opts->bitpos == LINEARCOMP_BITPOS_HIGH) {
        return obj->gi->nbits - 1;
    } else if (opts->bitpos == LINEARCOMP_BITPOS_MID) {
        return obj->gi->nbits / 2 - 1;
    } else if (opts->bitpos < 0) {
        return 0;
    } else {
        return (unsigned int) opts->bitpos;
    }
}

/**
 * @brief Linear complexity test based on Berlekamp-Massey algorithm.
 * @details Formula for p-value computation may be found in:
 *
 * 1. M.Z. Wang. Linear complexity profiles and jump complexity //
 *    // Information Processing Letters. 1997. V. 61. P. 165-168.
 *    https://doi.org/10.1016/S0020-0190(97)00004-5
 * 2. H. Gustafson, E. Dawson, L. Nielsen, W. Caelli. A computer package for
 *    measuring the strength of encryption algorithms // Computers & Security.
 *    1994. V.13. N 8. P.687-697. https://doi.org/10.1016/0167-4048(94)90051-5.
 * 3. Rukhin A., Soto J. et al. A Statistical Test Suite for Random and
 *    Pseudorandom Number Generators for Cryptographic Applications //
 *    NIST SP 800-22 Rev. 1a. https://doi.org/10.6028/NIST.SP.800-22r1a
 *
 * An empirical linear complexity has the next mathematical expectance and
 * variance [1]:
 * 
 * \f[
 * \mu = \frac{n}{2} + \frac{9 - (-1)^n}{36};~
 * \sigma = \sqrt{\frac{86}{81}}
 * \f]
 *
 * Gustafson et al. [2] proposed a normal approximation for it but
 * Rukhin et al.[3] noted that it is too crude, mainly because of heavier
 * tails and proposed to use the next integer \f$ T \f$ variable:
 *
 * \f[
 *   T = \begin{cases}
 *     L - \frac{n}{2}     & \textrm{for even } L \\
 *    -L + \frac{n + 1}{2} & \textrm{for odd } L \\
 *   \end{cases}
 * \f]
 *
 * It has the next c.d.f.:
 * \f[
 *   F(k) = \begin{cases}
 *     1 - \frac{2^{-2k + 2}}{3} & \textrm{for } k > 0 \\
 *         \frac{2^{2k + 1}}{3}  & \textrm{for } k \le 0 \\
 *   \end{cases}
 * \f]
 *
 * It is also possible to approximate the \f$(L - \mu)/\sigma\f$ value by
 * t-distribution with \f$ f=11 \f$ degrees of freedom. It is good in central
 * part but has too dense tails. The f value is obtained during the SmokeRand
 * development by the Monte-Carlo method by repeating the test 10^7 times
 * for n=1000,1001,2000,2001. Speck128/128 was used as a CSPRNG
 * (see `src/calibrate_linearcomp.c`).
 *
 * @param opts Test options (number of bits, bit position)
 */
TestResults linearcomp_test(GeneratorState *obj, const LinearCompOptions *opts)
{
    TestResults ans = TestResults_create("linearcomp");
    unsigned int bitpos = linearcomp_get_bitpos(obj, opts);
    uint8_t *s = calloc(opts->nbits, sizeof(uint8_t));
    if (s == NULL) {
        fprintf(stderr, "***** linearcomp_test: not enough memory *****\n");
        exit(EXIT_FAILURE);
    }
    obj->intf->printf("Linear complexity test\n");
    obj->intf->printf("  nbits: %lld; bitpos: %d\n",
        (long long) opts->nbits, (int) bitpos);
    uint64_t mask = 1ull << bitpos;
    for (size_t i = 0; i < opts->nbits; i++) {
        if (obj->gi->get_bits(obj->state) & mask)
            s[i] = 1;
    }
    ans.x = (double) berlekamp_massey(s, opts->nbits);
    double T;
    if (opts->nbits & 1) {
        T = -ans.x + (double) (opts->nbits + 1) / 2.0;
    } else {
        T = ans.x - (double) opts->nbits / 2.0;
    }
    ans.penalty = PENALTY_LINEARCOMP;
    ans.p = sr_linearcomp_Tcdf(T);
    ans.alpha = sr_linearcomp_Tccdf(T);
    obj->intf->printf("  L = %g; T = %g; p = %g\n", ans.x, T, ans.p);
    obj->intf->printf("\n");
    free(s);
    return ans;
}

///////////////////////////////////////////////
///// Interfaces (wrappers) for batteries /////
///////////////////////////////////////////////

TestResults linearcomp_test_wrap(GeneratorState *obj, const void *udata)
{
    return linearcomp_test(obj, udata);
}


TestResults matrixrank_test_wrap(GeneratorState *obj, const void *udata)
{
    return matrixrank_test(obj, udata);
}
