/**
 * @file lfsr_period.c
 * @brief Simple tools for proving the LFSR period using the theoretical
 * (algebraic) methods. Allow to check small xorshift-style generators with
 * states up to 1024 bits.
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * Some polynomial GF(2) arithmetics is based on the public domain code
 * by S. Vigna (https://prng.di.unimi.it/f2x.c).
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/lfsr_period.h"
#include "smokerand/lfsr_period_factors.h"
#include "smokerand/lineardep.h"
#include "smokerand/specfuncs.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

//////////////////////////////////////////
///// LargeInt class implemenetation /////
//////////////////////////////////////////

/**
 * @brief Create a large integer from the unsigned 64-bit value.
 */
LargeInt LargeInt_from_u64(uint64_t x)
{
    LargeInt obj;
    obj.x[0] = x;
    for (size_t i = 1; i < LARGEINT_SIZE; i++) {
        obj.x[i] = 0;
    }
    return obj;
}

/**
 * @brief Create a large integer from the \f$ 2^p \f$ value.
 * @param p  An exponent for the \f$ 2^p \f$ value.
 */
LargeInt LargeInt_from_pow2(unsigned int p)
{
    LargeInt obj;
    for (size_t i = 0; i < LARGEINT_SIZE; i++) {
        obj.x[i] = 0;
    }
    if (p < LARGEINT_SIZE * 64) {
        obj.x[p >> 6] = 1ULL << (p & 0x3FU);
    }
    return obj;
}

/**
 * @brief Subtract the unsigned 64-bit integer from the large integer.
 */
void LargeInt_subtract_u64(LargeInt *obj, uint64_t val)
{
    for (size_t i = 0; i < LARGEINT_SIZE; i++) {
        const uint64_t xi_old = obj->x[i];
        obj->x[i] -= val;
        if (obj->x[i] <= xi_old) {
            break;
        } else {
            val = 1;
        }
    }
}


int LargeInt_is_u64(const LargeInt *obj, uint64_t val)
{
    for (size_t i = LARGEINT_SIZE - 1; i >= 1; i--) {
        if (obj->x[i] != 0) {
            return 0;
        }
    }
    return obj->x[0] == val;
}


void LargeInt_div_2(LargeInt *obj)
{
    obj->x[0] >>= 1;
    for (size_t i = 1; i < LARGEINT_SIZE; i++) {
        const uint64_t hi = (obj->x[i] & 1U) << 63;
        obj->x[i] >>= 1;
        obj->x[i - 1] |= hi;
    }
}

/**
 * @brief Get the number of significant bits, i.e. without leading 0s.
 */
unsigned int LargeInt_get_nbits(const LargeInt *obj)
{
    unsigned int nbits = LARGEINT_SIZE * 64;
    for (size_t i = LARGEINT_SIZE; i-- != 0; ) {
        if (obj->x[i] == 0) {
            nbits -= 64;
        } else {
            const uint64_t mask = 0x8000000000000000U;
            uint64_t xi = obj->x[i];
            unsigned int leading_zeros = 0;
            while ((xi & mask) == 0) {
                xi <<= 1;
                leading_zeros++;
            }
            nbits -= leading_zeros;
            break;
        }
    }
    return nbits;
}


void LargeInt_print_hex(const LargeInt *obj, const CallerAPI *intf)
{
    int is_inside = 0;
    for (size_t i = LARGEINT_SIZE; i-- != 0; ) {
        if (!is_inside && obj->x[i] != 0) {
            is_inside = 1;
            intf->printf("%16.16llX", (unsigned long long) obj->x[i]);
        } else if (is_inside) {
            intf->printf(".%16.16llX", (unsigned long long) obj->x[i]);
        }
    }
}

/////////////////////////////////////////
///// LfsrPoly class implementation /////
/////////////////////////////////////////

LfsrPoly LfsrPoly_create(size_t degree)
{
    LfsrPoly obj;
    size_t nwords = degree >> 6;
    if ((degree & 0x3F) != 0 || degree == 0) {
        nwords++;
    }
    obj.w64 = calloc(nwords, sizeof(uint64_t));
    ASSERT_MALLOC_PTR(obj.w64, "LfsrPoly_create")
    obj.degree = degree;
    obj.nwords = nwords;
    return obj;
}


void LfsrPoly_print(const LfsrPoly *obj, const CallerAPI *intf)
{
    unsigned int nterms = 1;
    intf->printf("x^%u + ", (unsigned int) obj->degree);
    for (size_t i = obj->degree; i-- != 0; ) {
        if (LfsrPoly_getbit(obj, i) != 0) {
            if (i > 0) {
                intf->printf("x^%u + ", (unsigned int) i);
            } else {
                intf->printf("1");
            }
            nterms++;
        }
    }
    intf->printf(" | nterms = %u", nterms);
}

void LfsrPoly_print_hex(const LfsrPoly *obj, const CallerAPI *intf)
{
    for (size_t i = obj->nwords; i-- != 0; ) {
        intf->printf("%16.16llX%s",
            (unsigned long long) obj->w64[i],
            (i > 0) ? "." : ""
        );
    }
}


void LfsrPoly_print_carray(const LfsrPoly *obj, const CallerAPI *intf)
{
    for (size_t i = 0; i < obj->nwords; i++) {
        intf->printf("%s0x%16.16llX%s",
            (i == 0) ? "{" : "",
            (unsigned long long) obj->w64[i],
            (i < obj->nwords - 1) ? ", " : "}"
        );
    }
}


LfsrPoly LfsrPoly_clone(const LfsrPoly *obj)
{
    LfsrPoly out = LfsrPoly_create(obj->degree);
    memcpy(out.w64, obj->w64, obj->nwords * sizeof(uint64_t));
    return out;
}


void LfsrPoly_destruct(LfsrPoly *obj)
{
    free(obj->w64);
}


/**
 * @brief `a <- a * x mod charpoly`
 */
void LfsrPoly_mulx(LfsrPoly *a, const LfsrPoly *charpoly)
{
    const size_t degree = charpoly->degree;
    uint64_t carry = 0;
    if (a->nwords != charpoly->nwords) {
        return;
    }
    for (size_t i = 0; i < a->nwords; i++) {
        const uint64_t next_carry = a->w64[i] >> 63;
        a->w64[i] = (a->w64[i] << 1) | carry;
        carry = next_carry;
    }
    // Coefficient of x^POLY_DEG after the shift.
    int top;
    if (degree % 64 == 0) {
    	top = (int) carry;
    } else {
        top = (a->w64[degree >> 6] >> (degree & 63)) & 1;
        a->w64[degree >> 6] &= ~(UINT64_C(1) << (degree & 63));
    }
	if (top) {
        for (size_t i = 0; i < charpoly->nwords; i++) {
            a->w64[i] ^= charpoly->w64[i];
        }
    }

}

/**
 * @brief `a <- a * b mod charpoly` (Horner over the bits of a, from the top)
 */
void LfsrPoly_mulmod(LfsrPoly *a, const LfsrPoly *b, const LfsrPoly *charpoly)
{
    if (a->nwords != b->nwords || a->nwords != charpoly->nwords) {
        return;
    }
    LfsrPoly r = LfsrPoly_create(charpoly->degree);
    for (size_t k = charpoly->degree; k-- != 0; ) {
        LfsrPoly_mulx(&r, charpoly);
		if ((a->w64[k >> 6] >> (k & 63)) & 1) {
            for (size_t i = 0; i < a->nwords; i++) {
                r.w64[i] ^= b->w64[i];
            }
        }
	}
    memcpy(a->w64, r.w64, a->nwords*sizeof(uint64_t));
    LfsrPoly_destruct(&r);
}

/**
 * @brief `out <- x^(c * 2^e) mod charpoly`
 */
LfsrPoly LfsrPoly_jumppoly_ce(const LfsrPoly *charpoly, uint64_t c, uint32_t e)
{
    LfsrPoly out = LfsrPoly_create(charpoly->degree);
    out.w64[0] = 1; // out = 1
    for (int k = 63; k >= 0; k--) { // out = x^c
        LfsrPoly_mulmod(&out, &out, charpoly);
        if ((c >> k) & 1)
            LfsrPoly_mulx(&out, charpoly);
    }
    while (e--) {
        LfsrPoly_mulmod(&out, &out, charpoly); // out = (x^c)^(2^e) = x^(c * 2^e)
    }
    return out;
}

/**
 * @brief `out <- x^(2^e - 1) mod charpoly`
 */
LfsrPoly LfsrPoly_jumppoly_mersenne(const LfsrPoly *charpoly, uint32_t e)
{
    // out = x
    LfsrPoly out = LfsrPoly_create(charpoly->degree); out.w64[0] = 1;
    LfsrPoly_mulx(&out, charpoly);
    // sq = x^2 mod charpoly
    LfsrPoly sq  = LfsrPoly_clone(&out);
    LfsrPoly_mulmod(&sq, &sq, charpoly);

    for (uint32_t i = 1; i < e; i++) {
        LfsrPoly_mulmod(&out, &sq, charpoly);
        LfsrPoly_mulmod(&sq,  &sq, charpoly);
    }
    LfsrPoly_destruct(&sq);
    return out;
}


/**
 * @brief `out <- x^n mod charpoly`, where `n = x[0] + x[1] * 2^64 + ...` is the
 * little-endian integer held in the len words of jump (square-and-multiply
 * over the bits of n, from the most significant down).
 */
LfsrPoly LfsrPoly_jumppoly_n(const LfsrPoly *charpoly, const LargeInt *n)
{
    LfsrPoly out = LfsrPoly_create(charpoly->degree);
    out.w64[0] = 1; // out = 1

    const int nbits = (int) LargeInt_get_nbits(n);
    for (int k = nbits - 1; k >= 0; k--) { // out = x^n
        LfsrPoly_mulmod(&out, &out, charpoly);
        if ((n->x[k >> 6] >> (k & 63)) & 1)
            LfsrPoly_mulx(&out, charpoly);

    }
    return out;
}


int LfsrPoly_is_one(const LfsrPoly *obj)
{
    if (obj->nwords == 0 || obj->w64[0] != 1) {
        return 0;
    } else {
        for (size_t i = 1; i < obj->nwords; i++) {
            if (obj->w64[i] != 0) {
                return 0;
            }
        }
        return 1;
    }
}


int LfsrPoly_is_period_possible(const LfsrPoly *charpoly, const LargeInt *period)
{
    LfsrPoly jumppoly = LfsrPoly_jumppoly_n(charpoly, period);
    const int is_possible = LfsrPoly_is_one(&jumppoly);
    LfsrPoly_destruct(&jumppoly);
    return is_possible;
}


int LfsrPoly_is_mersenne_period_possible(const LfsrPoly *charpoly, uint32_t e)
{
    LfsrPoly jumppoly = LfsrPoly_jumppoly_mersenne(charpoly, e);
    const int is_possible = LfsrPoly_is_one(&jumppoly);
    LfsrPoly_destruct(&jumppoly);
    return is_possible;
}


///////////////////////////////////////////
///// LfsrMatrix class implementation /////
///////////////////////////////////////////

LfsrMatrix LfsrMatrix_create(size_t n)
{
    LfsrMatrix obj;
    if (n > 0) {
        obj.x = calloc(n * n, sizeof(uint8_t));
    } else {
        obj.x = calloc(1, sizeof(uint8_t));
    }
    ASSERT_MALLOC_PTR(obj.x, "LfsrMatrix_create")
    obj.n = n;
    return obj;
}

/**
 * @brief Matrix multiplication in the GF(2) field.
 * @details It uses rows and columns packing to 64-bit words, bitwise operations
 * and Hamming weights compuation to optimize the multiplication (important for
 * matrices with n >= 128).
 * @param a The first matrix.
 * @param b The second matrix.
 * @return The matrix product, must be destructed by the caller.
 */
LfsrMatrix LfsrMatrix_create_prod(const LfsrMatrix *a, const LfsrMatrix *b)
{
    // Check the matrices size
    const size_t n = a->n;
    if (b->n != n) {
        LfsrMatrix c = LfsrMatrix_create(0);
        return c;
    }
    // Cache rows/columns
    // It is O(n^2) and the matrix multiplication is O(n^3)
    size_t nwords = n / 64;
    if (n % 64 > 0) {
        nwords++;
    }
    uint64_t *arows = calloc(nwords * n, sizeof(uint64_t));
    ASSERT_MALLOC_PTR(arows, "LfsrMatrix_create_prod")
    uint64_t *bcols = calloc(nwords * n, sizeof(uint64_t));
    ASSERT_MALLOC_PTR(bcols, "LfsrMatrix_create_prod")
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            const uint64_t aij = LfsrMatrix_getbit(a, i, j);
            const uint64_t bij = LfsrMatrix_getbit(b, i, j);
            arows[i*nwords + (j >> 6)] |= aij << (j & 0x3FU);
            bcols[j*nwords + (i >> 6)] |= bij << (i & 0x3FU);
        }
    }    
    // The multiplication procedure
    // It is based on the cij ^= aik & bki formula but uses bitwise
    // operations on 64-bit words and Hamming weights for optimization.
    LfsrMatrix c = LfsrMatrix_create(n);
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            uint8_t cij = 0;
            for (size_t k = 0; k < nwords; k++) {
                const uint64_t prods = arows[i*nwords + k] & bcols[j*nwords + k];
                cij = (uint8_t) (cij + get_uint64_hamming_weight(prods));
            }
            LfsrMatrix_setbit(&c, i, j, cij & 1);
        }
    }
    // Free buffers and return the resulting matrix
    free(arows);
    free(bcols);
    return c;
}

/**
 * @brief Check if the two matrices are equal.
 */
int LfsrMatrix_are_equal(const LfsrMatrix *a, const LfsrMatrix *b)
{
    if (a->n != b->n) {
        return 0;
    }
    for (size_t i = 0; i < a->n; i++) {
        for (size_t j = 0; j < a->n; j++) {
            if (LfsrMatrix_getbit(a, i, j) != LfsrMatrix_getbit(b, i, j)) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * @brief Check if the matrix is the eye matrix.
 */
int LfsrMatrix_is_eye(const LfsrMatrix *a)
{
    for (size_t i = 0; i < a->n; i++) {
        for (size_t j = 0; j < a->n; j++) {
            const uint8_t dij = (i == j) ? 1 : 0;
            if (LfsrMatrix_getbit(a, i, j) != dij) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * @brief Make a copy of the matrix.
 */
LfsrMatrix LfsrMatrix_clone(const LfsrMatrix *obj)
{
    const size_t n = obj->n;
    LfsrMatrix cpy = LfsrMatrix_create(n);
    memcpy(cpy.x, obj->x, n*n);
    return cpy;
}

/**
 * @brief Destructor: deallocates all internal buffers but
 * not the structure itself.
 */
void LfsrMatrix_destruct(LfsrMatrix *obj)
{
    free(obj->x);
}

/**
 * @brief Calculate the matrix power.
 * @details It uses the classic "high-to-low" fast exponentation algorithm
 * described in [1], see Table 10.5, Chapter 10.
 *
 * References:
 *
 * 1. J.-P. Aumasson. Serious Cryptography. A practical introduction to modern
 *    encryption. No Starch Press. 2018. ISBN 978-1-59327-826-7.
 *
 * @param x  Matrix (base)
 * @param e  Exponent
 * @return The matrix power, must be destructed by the caller.
 */
LfsrMatrix LfsrMatrix_create_pow(const LfsrMatrix *x, const LargeInt *e)
{
    if (LargeInt_is_u64(e, 1)) {
        return LfsrMatrix_clone(x);
    } else if (LargeInt_is_u64(e, 2)) {
        return LfsrMatrix_create_prod(x, x);
    } else {
        LfsrMatrix y = LfsrMatrix_clone(x);
        const unsigned int nbits = LargeInt_get_nbits(e);
        for (unsigned int i = nbits - 1; i-- != 0; ) {
            // y = y * y
            LfsrMatrix sq = LfsrMatrix_create_prod(&y, &y);
            LfsrMatrix_destruct(&y);
            y = sq;
            if (LargeInt_getbit(e, i)) {
                // y = y * x
                LfsrMatrix yx = LfsrMatrix_create_prod(&y, x);
                LfsrMatrix_destruct(&y);
                y = yx;
            }
        }
        return y;
    }
}


typedef enum {
    LFSR_TILE_ZERO,
    LFSR_TILE_EYE,
    LFSR_TILE_OTHER
} LfsrMatrixTileType;


static LfsrMatrixTileType
LfsrMatrix_get_tile_type(const LfsrMatrix *obj, size_t i, size_t j, size_t width)
{
    // Check if there are 1s outside the main diagonal
    const size_t i_offset = i*width, j_offset = j*width;
    for (size_t ii = 0; ii < width; ii++) {
        for (size_t jj = 0; jj < width; jj++) {
            if (ii != jj && LfsrMatrix_getbit(obj, ii + i_offset, jj + j_offset) != 0) {
                return LFSR_TILE_OTHER;
            }
        }
    }
    // Check if the tile is zero or eye matrix
    unsigned int all_zeros = 1, all_ones = 1;
    for (size_t ii = 0; ii < width; ii++) {
        const uint8_t b = LfsrMatrix_getbit(obj, ii + i_offset, ii + j_offset);
        all_zeros = all_zeros && !b;
        all_ones  = all_ones  && b;
    }
    if (all_zeros) {
        return LFSR_TILE_ZERO;
    } else if (all_ones) {
        return LFSR_TILE_EYE;
    } else {
        return LFSR_TILE_OTHER;
    }
}


void LfsrMatrix_print(const LfsrMatrix *obj, const CallerAPI *intf)
{
    const size_t tile_width = 32;
    if (obj->n <= tile_width || obj->n % tile_width != 0) {
        for (size_t i = 0; i < obj->n; i++) {
            intf->printf("    |");
            for (size_t j = 0; j < obj->n; j++) {
                intf->printf("%s", LfsrMatrix_getbit(obj, i, j) ? "X" : ".");
            }
        intf->printf("|\n");
        }
    } else {
        const size_t ntiles = obj->n / tile_width;
        char *strbuf = calloc(ntiles + 8, sizeof(char));
        ASSERT_MALLOC_PTR(strbuf, "LfsrMatrix_print")
        for (size_t i = 0; i < ntiles; i++) {
            strcpy(strbuf, "    |");
            for (size_t j = 0; j < ntiles; j++) {
                const LfsrMatrixTileType t = LfsrMatrix_get_tile_type(obj, i, j, tile_width);
                switch(t) {
                case LFSR_TILE_ZERO:
                    strcat(strbuf, ".");
                    break;
                case LFSR_TILE_EYE:
                    strcat(strbuf, "I");
                    break;
                case LFSR_TILE_OTHER:
                default:
                    strcat(strbuf, "?");
                }
            }
            strcat(strbuf, "|\n");
            intf->printf(strbuf);
        }
        free(strbuf);
    }
}



int LfsrMatrix_is_period_possible(const LfsrMatrix *mat, const LargeInt *period)
{
    LfsrMatrix matp = LfsrMatrix_create_pow(mat, period);
    const int is_possible = LfsrMatrix_is_eye(&matp);
    LfsrMatrix_destruct(&matp);
    return is_possible;
}

/**
 * @brief Converts LFSR transition matrix to Krylov matrix.
 */
LfsrMatrix LfsrMatrix_get_krylov_matrix(const LfsrMatrix *mat)
{
    const size_t nbits = mat->n;
    LfsrMatrix kmat = LfsrMatrix_create(nbits + 1);
    LfsrMatrix_setbit(&kmat, 0, 0, 1);
    for (size_t i = 0; i < nbits; i++) {
        for (size_t j = 0; j < nbits; j++) {
            uint8_t b = 0;
            for (size_t k = 0; k < nbits; k++) {
                const uint8_t u = LfsrMatrix_getbit(mat, k, j);
                const uint8_t v = LfsrMatrix_getbit(&kmat, i, k);
                b = (uint8_t) (b ^ (u & v));
            }
            LfsrMatrix_setbit(&kmat, i + 1, j, b);
        }
    }
    return kmat;
}

/**
 * @brief Convert LFSR Krylov matrix to its characteristic polynomial.
 * The converted matrix will be altered by Gaussian elimination.
 * @details Krylov matrix has the \f$ (n+1)\times (n+1) \f$ size (where
 * n is the number of bits in the LFSR state) and should have the next
 * layout:
 *
 *     | -----  x  ----- 0 |
 *     | ----- xA  ----- 0 |
 *     | ----- xA^2 ---- 0 |
 *     |    ..........     |
 *     | ----- xA^n ---- 0 |
 *
 * where A is the LFSR transition matrix and x is the initial LFSR state.
 * Note that x is the row vector (just as in papers by Marsaglia, Blackman
 * and Vigna).
 *
 * The code solves the next system of linear equations in the GF(2) field:
 *
 *
 *                       | -----  x  ----- |
 *                       | ----- xA  ----- |
 *     |c0 c1 ... cn| =  | ----- xA^2 ---- | = | ----- xA^n ----- |
 *                       |    ..........   |
 *                       | ----- xA^n ---- |
 *
 * Characteristic polynomial can be interpreted as the next recurrent formula
 * for some bit of the xorshift-like LFSR output:
 *
 * \f[
 * c_0 b_0 \oplus c_1 b_1 \oplus \ldots \oplus c_n b_n = 0
 * \f]
 */
LfsrPoly LfsrMatrix_krylov_to_charpoly(LfsrMatrix *mat)
{
    const size_t nbits = mat->n - 1;
    // Gaussian elimination. Note: each equation is a column!
    // a) Initialize the columns indexex for its swapping
    size_t *cinds = calloc(nbits + 1, sizeof(size_t));
    ASSERT_MALLOC_PTR(cinds, "LfsrMatrix_krylov_to_charpoly")
    for (size_t i = 0; i < nbits + 1; i++) {
        cinds[i] = i;
    }
    // b) The method itself
    for (size_t j = 0; j < nbits; j++) {
        // b1) pivot
        size_t i_pivot;
        for (i_pivot = j;
             LfsrMatrix_getbit(mat, j, cinds[i_pivot]) == 0 && i_pivot < nbits;
             i_pivot++) {
            
        }
        if (i_pivot < nbits) {
            const size_t ind = cinds[i_pivot];
            cinds[i_pivot] = cinds[j];
            cinds[j] = ind;
        } else {
            free(cinds);
            return LfsrPoly_create_invalid();
        }
        // b2) Elimination
        // Note: LfsrMatrix_getbit(&mat, j, cinds[j])) is always 1
        for (size_t i = 0; i < nbits; i++) {
            if (i != j && LfsrMatrix_getbit(mat, j, cinds[i]) != 0) {
                // Add column cinds[j] to cinds[i]
                for (size_t k = 0; k < nbits + 1; k++) {
                    const uint8_t a = (uint8_t) (LfsrMatrix_getbit(mat, k, cinds[i]));
                    const uint8_t b = (uint8_t) (LfsrMatrix_getbit(mat, k, cinds[j]));
                    LfsrMatrix_setbit(mat, k, cinds[i], (uint8_t) (a ^ b));
                }
            }
        }
    }
    // c) Restore the polynomial
    LfsrPoly poly = LfsrPoly_create(nbits);
    for (size_t i = 0; i < nbits; i++) {
        if (LfsrMatrix_getbit(mat, nbits, cinds[i])) {
            LfsrPoly_setbit(&poly, i);
        }
    }
    free(cinds);
    return poly;
}


//////////////////////////////////////////////////
///// GeneratorStateExt class implementation /////
//////////////////////////////////////////////////

static unsigned int malloc_ncalls = 0;
static size_t malloc_nbytes = 0;
static CallerAPI intf_hooked;
static void *(*malloc_original)(size_t len);


static void *malloc_loghook(size_t len)
{
    malloc_ncalls++;
    malloc_nbytes += len;
    return malloc_original(len);
}

/**
 * @brief Creates the PRNG example with measurement of its state size.
 * The `intf->malloc` hook is used. WARNING: this function is not
 * thread safe!
 * @details WARNING: this function is not reentrant (not thread safe) because
 * it uses `intf->malloc` hooks to measure the PRNG state size.
 */
GeneratorStateExt
GeneratorStateExt_create(const GeneratorInfo *gen, const CallerAPI *intf)
{
    malloc_ncalls = 0;
    malloc_nbytes = 0;
    malloc_original = intf->malloc;
    intf_hooked = *intf;
    intf_hooked.malloc = malloc_loghook;
    GeneratorStateExt ext;
    ext.state = GeneratorState_create(gen, &intf_hooked);
    ext.nbytes = (malloc_ncalls == 1) ? malloc_nbytes : 0;
    return ext;
}


GeneratorStateExt
GeneratorStateExt_create_sized(const GeneratorInfo *gen, const CallerAPI *intf, size_t nbytes)
{
    if (nbytes == LFSR_NBYTES_DEFAULT) {
        return GeneratorStateExt_create(gen, intf);
    } else {
        GeneratorStateExt ext;
        ext.state = GeneratorState_create(gen, intf);
        ext.nbytes = nbytes;
        return ext;
    }
}

/**
 * @brief Restores the LFSR transition matrix using only its transition function
 * (that returns pseudorandom values) and initialization with states like
 * `[100000]`, `[010000]`, `[001000]` etc.
 * @details This matrix uses slightly unusual conventions taken from [1]: it
 * is applied as \f$ x_{n+1} = x_{n} A \f$ where \f$x_{n}\f$ is a *row* bit
 * vector representing the generator state.
 *
 * 1. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * @param obj The generator to be analyzed.
 * @param niters Number of iterations before the matrix recovery:
 *               niters=1 will restore A^1, niters=2 - A^2 etc.
 * @return The LFSR transition matrix over the GF(2) field.
 */
LfsrMatrix
GeneratorStateExt_get_matrix(GeneratorStateExt *obj, unsigned long long niters)
{
    const size_t nbits = obj->nbytes * 8;
    LfsrMatrix mat = LfsrMatrix_create(nbits);
    uint8_t *buf = obj->state.state;
    for (size_t i = 0; i < nbits; i++) {
        memset(buf, 0, obj->nbytes);
        buf[i >> 3] = (uint8_t) (1U << (i & 0x7U));
        for (unsigned long long j = 0; j < niters; j++) {
            (void) obj->state.gi->get_bits(obj->state.state);
        }
        for (size_t j = 0; j < nbits; j++) {
            const uint8_t b = (uint8_t) (buf[j >> 3] & (1U << (j & 0x7U)));
            LfsrMatrix_setbit(&mat, i, j, b);
        }
    }
    return mat;
}

/**
 * @brief Restores Krylov matrix required for computation of the primitive
 * polynomial that corresponds to the LFSR.
 */
LfsrMatrix GeneratorStateExt_get_krylov_matrix(GeneratorStateExt *obj)
{
    const size_t nbits = obj->nbytes * 8;
    LfsrMatrix mat = LfsrMatrix_create(nbits + 1);
    uint8_t *buf = obj->state.state;
    // Set an initial state of the generator
    for (size_t i = 0; i < obj->nbytes; i++) {
        buf[i] = (uint8_t) ((i % 2) ? 0x55 : 0xAA);
    }
    // Generate the system of equation (based on Krylov space)
    // Use the row vectors here
    for (size_t i = 0; i < nbits + 1; i++) {
        for (size_t j = 0; j < nbits; j++) {
            const uint8_t b = (uint8_t) (buf[j >> 3] & (1U << (j & 0x7U)));
            LfsrMatrix_setbit(&mat, i, j, b);
        }
        (void) obj->state.gi->get_bits(obj->state.state);
    }
    return mat;
}


/**
 * @brief Restores the primitive characteristic polynomial of the LFSR.
 */
LfsrPoly GeneratorStateExt_get_poly(GeneratorStateExt *obj)
{
    LfsrMatrix mat = GeneratorStateExt_get_krylov_matrix(obj);
    LfsrPoly poly = LfsrMatrix_krylov_to_charpoly(&mat);
    LfsrMatrix_destruct(&mat);
    return poly;
}




/**
 * @brief Restores the jump polynomial that corresponds to the jump matrix
 * of the LFSR.
 */
LfsrPoly GeneratorStateExt_get_jump_poly_pow2(GeneratorStateExt *obj, unsigned int p)
{
    LfsrPoly char_poly = GeneratorStateExt_get_poly(obj);
    LfsrPoly jump_poly = LfsrPoly_jumppoly_ce(&char_poly, 1, p);
    LfsrPoly_destruct(&char_poly);
    return jump_poly;
}

/**
 * @brief Makes a jump in an assumption that PRNG is an LFSR using a supplied
 * jump polynomial.
 */
void GeneratorStateExt_apply_jump_poly(GeneratorStateExt *obj, const LfsrPoly *jump_poly)
{
    uint8_t *new_state = calloc(obj->nbytes, sizeof(uint8_t));
    ASSERT_MALLOC_PTR(new_state, "GeneratorStateExt_apply_jump_poly")
    uint8_t *cur_state = obj->state.state;
    for (size_t i = 0; i < jump_poly->degree; i++) {
        if (LfsrPoly_getbit(jump_poly, i)) {
            for (size_t j = 0; j < obj->nbytes; j++) {
                new_state[j] ^= cur_state[j];
            }
        }
        (void) obj->state.gi->get_bits(obj->state.state);
    }
    memcpy(cur_state, new_state, obj->nbytes);
    free(new_state);
}

/**
 * @brief Makes a jump in an assumption that PRNG is an LFSR using a supplied
 * jump polynomial.
 */
void GeneratorStateExt_make_jump_pow2(GeneratorStateExt *obj, unsigned int p)
{
    uint8_t *old_state = malloc(obj->nbytes);
    ASSERT_MALLOC_PTR(old_state, "GeneratorStateExt_make_jump_pow2")
    memcpy(old_state, obj->state.state, obj->nbytes);
    LfsrPoly jump_poly = GeneratorStateExt_get_jump_poly_pow2(obj, p);
    memcpy(obj->state.state, old_state, obj->nbytes);
    free(old_state);
    GeneratorStateExt_apply_jump_poly(obj, &jump_poly);
    LfsrPoly_destruct(&jump_poly);
}


/**
 * @brief Check if the PRNG is LFSR or not.
 */
int GeneratorStateExt_is_lfsr(GeneratorStateExt *obj)
{
    const unsigned long niters = 65537;
    const LargeInt niters_lint = LargeInt_from_u64(niters);
    LfsrMatrix mat = GeneratorStateExt_get_matrix(obj, 1);
    LfsrMatrix matp_exp = GeneratorStateExt_get_matrix(obj, niters);
    LfsrMatrix matp_calc = LfsrMatrix_create_pow(&mat, &niters_lint);

    const int is_lfsr = LfsrMatrix_are_equal(&matp_exp, &matp_calc);

    LfsrMatrix_destruct(&mat);
    LfsrMatrix_destruct(&matp_exp);
    LfsrMatrix_destruct(&matp_calc);

    return is_lfsr;
}

/**
 * @brief Check if PRNG has counters and/or constants. It is important
 * to prevent memory corruption and segmentation fault during the 
 * period deduction attempts.
 */
int GeneratorStateExt_has_counters(GeneratorStateExt *obj)
{
    const unsigned long niters = 10000000;
    const size_t nbytes = obj->nbytes;
    uint8_t *prev = malloc(nbytes);
    ASSERT_MALLOC_PTR(prev, "GeneratorStateExt_has_counters")
    uint8_t *is_byte_ctr = malloc(nbytes);
    ASSERT_MALLOC_PTR(is_byte_ctr, "GeneratorStateExt_has_counters")
    memset(is_byte_ctr, 1, nbytes);
    // Check if any bytes behave like a counter
    for (unsigned long i = 0; i < niters; i++) {
        // Iterate the PRNG state
        const uint8_t *cur = obj->state.state;
        memcpy(prev, obj->state.state, nbytes);
        (void) obj->state.gi->get_bits(obj->state.state);
        // Check if some bytes are not counters
        for (size_t j = 0; j < nbytes; j++) {
            const uint8_t delta = (uint8_t) (cur[j] - prev[j]);
            if (delta != 0 && delta != 1) {
                is_byte_ctr[j] = 0;
            }
        }
    }
    // Get the final result, free buffers and finish
    int has_ctr = 0;
    for (size_t i = 0; i < nbytes; i++) {
        if (is_byte_ctr[i]) {
            has_ctr = 1;
            break;
        }
    }
    free(prev);
    free(is_byte_ctr);
    return has_ctr;
}

/**
 * @brief Check if the generator is valid, i.e. is LFSR and has no counters or
 * constants inside its state.
 */
int GeneratorStateExt_is_valid(GeneratorStateExt *obj, const CallerAPI *intf)
{
    // Check if the PRNG has counters (by empirical testing)
    if (GeneratorStateExt_has_counters(obj)) {
        intf->printf("  The PRNG has constants and/or counters inside its state. Automated\n");
        intf->printf("  period deduction is impossible and may cause memory corruption\n");
        intf->printf("  and/or segmentation fault.\n");
        return 0;
    }

    // Check if the PRNG is LFSR (by empirical testing)
    if (GeneratorStateExt_is_lfsr(obj)) {
        intf->printf("  The PRNG is probably a LFSR\n");
    } else {
        intf->printf("  The PRNG is not a LFSR\n");
        return 0;
    }
    // The generator is valid
    return 1;
}


void GeneratorStateExt_destruct(GeneratorStateExt *obj)
{
    GeneratorState_destruct(&(obj->state));
}

////////////////////////////////////
///// Battery implemenentation /////
////////////////////////////////////

void LfsrPeriodResult_print(const CallerAPI *intf, LfsrPeriodResult res)
{
    switch (res) {
    case LFSR_PERIOD_MAX:
        intf->printf("The LFSR has a maximal period\n");
        break;
    case LFSR_PERIOD_NOT_MAX:
        intf->printf("The LFSR period is not maximal\n");      
        break;
    case LFSR_PERIOD_ERROR:
    default:
        intf->printf("The verification cannot be applied to this PNG\n");
    }
}


static LfsrPeriodResult lfsr_period_test_core(const LfsrPoly *charpoly,
    unsigned int nbits, const CallerAPI *intf)
{
    // Check the A^period = I condition
    LfsrPeriodResult result = LfsrPoly_is_mersenne_period_possible(charpoly, nbits) ?
                              LFSR_PERIOD_MAX : LFSR_PERIOD_NOT_MAX;
    if (result == LFSR_PERIOD_MAX) {
        intf->printf("  A^period = I: passed\n");
    } else {
        intf->printf("  A^period = I: failed\n");
        return LFSR_PERIOD_NOT_MAX;
    }
    // Check the A^(period/pi) <> I condition
    const LargeInt *lfsr_exps = get_lfsr_exps(nbits);
    if (lfsr_exps == NULL) {
        intf->printf("  The tables are absent for this LFSR size\n");
        result = LFSR_PERIOD_ERROR;
    } else {
        intf->printf("  Verifying the A^(period/prime) = A^e <> I exponents\n");
        for (const LargeInt *d = lfsr_exps; !LargeInt_is_u64(d, 0); d++) {
            intf->printf("  Exponent (%4u bits): ", LargeInt_get_nbits(d));
            LargeInt_print_hex(d, intf);
            // Use LfsrMatrix_is_period_possible if old method is desired
            if (LfsrPoly_is_period_possible(charpoly, d)) {
                intf->printf(" <<< FAIL\n");
                result = LFSR_PERIOD_NOT_MAX;
            } else {
                intf->printf(" OK\n");
            }
        }
    }
    return result;
}

/**
 * @brief This test verifies if a PRNG is a LFSR with the maximal period. It
 * supports LFSRs with 32, 64, 96, 128, 160, 192, 256, 320, 512, 800, 1024 and
 * 1600 bits of state. Works only with PRNG plugins, not with stdin/stdout.
 * @details The PRNG period verification is algebraic (analytical) and
 * actively modifies the generator internal state. It uses the next algorithm:
 *
 * 1. Create a PRNG example and define its state size by hooking the
 *    `intf->malloc` function. The generator should allocate only one
 *    memory region during initialization: its state.
 * 2. Check if the generator has constants or counters inside its state.
 *    If it has them - the test is interrupted to prevent memory corruption.
 *    Used heuristics are rather "fool proof" but always remember about
 *    the "better idiot" from Murphy laws.
 * 3. Restore the LFSR transition matrix over GF(2) field, see the
 *    GeneratorStateExt_get_matrix function documentation.
 * 4. Check if the PRNG is a really LFSR, see the GenertorStateExt_is_lfsr
 *    function. It will compare theoretical and empirical transition
 *    matrices obtained after several itertations.
 * 5. Check the \f$ A^{m} = I \f$ condition when \f$ m \f$ is the PRNG period.
 * 6. Check the \f$ A^{\frac{m}{p_i}} \neq I \f$ conditions where \f$ p_i \f$
 *    are prime divisors of \f$ m \f$.
 *
 * References:
 *
 * 1. Brent R.P. Some long-period random number generators using shifts and xors
 *   // ANZIAM J. 2007. V.48. P. C188--C202. https://doi.org/10.21914/anziamj.v48i0.40
 *   Proceedings of the 13th Biennial Computational Techniques and Applications
 *   Conference, CTAC-2006. Editors: Wayne Read  and A. J. Roberts
 * 2. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 */
LfsrPeriodResult lfsr_period_test(GeneratorStateExt *ext, const CallerAPI *intf,
    const LfsrPeriodOptions *opts)
{
    intf->printf("LFSR period checker\n");
    intf->printf("  malloc: nbytes = %llu; ptr = 0x%llu\n",
        (unsigned long long) ext->nbytes,
        (unsigned long long) (size_t) ext->state.state);
    // Check the generator validity
    if (opts->check_validity && !GeneratorStateExt_is_valid(ext, intf)) {
        LfsrPeriodResult_print(intf, LFSR_PERIOD_ERROR);
        return LFSR_PERIOD_ERROR;
    }
    // Calculate the maximal period
    const unsigned int nbits = (unsigned int) (ext->nbytes * 8);
    intf->printf("  The maximal period to be verified: 2**%u - 1\n", nbits);
    // Check if the maximal period is possible
    intf->printf("Beginning the period verification\n");
    LfsrMatrix mat = GeneratorStateExt_get_matrix(ext, 1);
    intf->printf("  LFSR transition matrix layout:\n");
    LfsrMatrix_print(&mat, intf);
    LfsrPoly charpoly = GeneratorStateExt_get_poly(ext);
    LfsrPeriodResult result = LFSR_PERIOD_ERROR;
    if (!LfsrPoly_is_valid(&charpoly)) {
        intf->printf("  Characteristic polynomial degree is lower than expected\n");
        result = LFSR_PERIOD_NOT_MAX;
    } else {
        result = lfsr_period_test_core(&charpoly, nbits, intf);
    }

    LfsrPeriodResult_print(intf, result);
    LfsrMatrix_destruct(&mat);
    LfsrPoly_destruct(&charpoly);
    return result;
}

///////////////////////////////////////////////////////////////////
///// An alternative test based on Berlekamp-Massey algorithm /////
///////////////////////////////////////////////////////////////////



LfsrPeriodResult lfsr_period_bm_test(GeneratorState *obj, const CallerAPI *intf)
{
    const size_t nbits_max = 200000;
    intf->printf("LFSR period checker (Berlekamp-Massey algorithm)\n");
    // Restore the possible characteristic polynomial
    uint8_t *charpoly_bits;
    uint8_t *s = extract_bits_from_pos(obj, nbits_max, 0);
    const size_t degree = berlekamp_massey(s, nbits_max, &charpoly_bits);
    const double p_value = sr_linearcomp_Tcdf((double) linearcomp_L_to_T(degree, nbits_max));
    if (1e-15 < p_value && p_value < 1 - 1e-15) {
        free(s);
        free(charpoly_bits);
        intf->printf("  The PRNG is either not a LFSR or has too large state\n");
        intf->printf("  degree = %llu; nbits = %llu; p = %g\n",
            (unsigned long long) degree, (unsigned long long) nbits_max, p_value);
        return LFSR_PERIOD_ERROR;        
    }
    LfsrPoly charpoly = LfsrPoly_create(degree);
    for (size_t i = 0; i < degree; i++) {
        if (charpoly_bits[i] != 0) {        
            LfsrPoly_setbit(&charpoly, i);
        }
    }
    free(s);
    free(charpoly_bits);
    // Show the polynomial
    intf->printf("Characteristic polynomial:\n");
    if (degree <= 1024) {
        LfsrPoly_print(&charpoly, intf); intf->printf("\n");
    }
    intf->printf("  ");
    LfsrPoly_print_hex(&charpoly, intf); intf->printf("\n");
    intf->printf("  uint64_t char_poly[] = ");
    LfsrPoly_print_carray(&charpoly, intf); intf->printf(";\n");
    // Calculate the maximal period
    const unsigned int nbits = (unsigned int) (degree);
    intf->printf("  The maximal period to be verified: 2**%u - 1\n", nbits);
    // Check if the maximal period is possible
    const LfsrPeriodResult result = lfsr_period_test_core(&charpoly, nbits, intf);

    LfsrPeriodResult_print(intf, result);
    LfsrPoly_destruct(&charpoly);


    return result;
}



//////////////////////
///// Interfaces /////
//////////////////////

static BatteryExitCode LfsrPeriodResult_to_exitcode(LfsrPeriodResult res)
{
    switch (res) {
    case LFSR_PERIOD_MAX:
        return BATTERY_PASSED;
    case LFSR_PERIOD_NOT_MAX:
        return BATTERY_FAILED;
    case LFSR_PERIOD_ERROR:
        return BATTERY_ERROR;
    default:
        return BATTERY_ERROR;
    }
}

/**
 * @brief An envelope for the lfsr_period_test function that allows to call
 * it as a pseudo-battery. However it doesn't return p-value because they are
 * senseless for algebraic/analytical verification that is deterministic.
 * @details
 * WARNING: this function is not reentrant (not thread safe) because it uses
 * `intf->malloc` hooks to measure the PRNG state size.
 */
BatteryExitCode battery_lfsr_period(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts)
{
    (void) opts;
    const LfsrPeriodOptions test_opts = {.check_validity = 1};
    if (gen->parent != NULL) {
        intf->printf("  LFSR period checker error: cannot analyze an enveloped generator");
        return BATTERY_ERROR;
    }
    const time_t tic = time(NULL);

    GeneratorStateExt ext = GeneratorStateExt_create(gen, intf);
    const LfsrPeriodResult res = lfsr_period_test(&ext, intf, &test_opts);
    if (res == LFSR_PERIOD_MAX) {
        {
            LfsrPoly poly = GeneratorStateExt_get_poly(&ext);
            intf->printf("Characteristic polynomial:\n");
            LfsrPoly_print(&poly, intf); intf->printf("\n");
            intf->printf("  ");
            LfsrPoly_print_hex(&poly, intf); intf->printf("\n");
            intf->printf("  uint64_t char_poly[] = ");
            LfsrPoly_print_carray(&poly, intf); intf->printf(";\n");
            LfsrPoly_destruct(&poly);
        }
        {
            const unsigned int jump_pow2 = (unsigned int) (ext.nbytes * 4);
            LfsrPoly jump_poly = GeneratorStateExt_get_jump_poly_pow2(&ext, jump_pow2);
            intf->printf("Jump polynomial for the 2^%u jump:\n", jump_pow2);
            intf->printf("  ");
            LfsrPoly_print_hex(&jump_poly, intf); intf->printf("\n");
            intf->printf("  uint64_t jump_poly[] = ");
            LfsrPoly_print_carray(&jump_poly, intf); intf->printf(";\n");
            LfsrPoly_destruct(&jump_poly);
        }

    }
    GeneratorStateExt_destruct(&ext);
    // Estimate the elapsed time
    char elapsed_time_txt[16];    
    snprintf_elapsed_time(elapsed_time_txt, 15, (unsigned long long) (time(NULL) - tic));
    intf->printf("Time elapsed: %s\n", elapsed_time_txt);
    // Results interpretation
    return LfsrPeriodResult_to_exitcode(res);
}


BatteryExitCode battery_lfsr_bm_period(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts)
{
    (void) opts;
    GeneratorState obj = GeneratorState_create(gen, intf);
    const LfsrPeriodResult res = lfsr_period_bm_test(&obj, intf);
    
    GeneratorState_destruct(&obj);
    // Results interpretation
    return LfsrPeriodResult_to_exitcode(res);
}
