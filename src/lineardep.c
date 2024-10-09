#include "smokerand/lineardep.h"
#include "smokerand/smokerand_core.h"
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



TestResults matrixrank_test(GeneratorState *obj, size_t n, unsigned int max_nbits)
{
    int nmat = 32, Oi[3] = {0, 0, 0};
    double pi[3] = {0.1284, 0.5776, 0.2888};
    size_t mat_len = n * n / 32;
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
    obj->intf->printf("  x = %g; p = %g; 1-p = %g\n\n", ans.x, ans.p, ans.alpha);
    return ans;
}
