/**
 * @file sr_tiny.c
 * @brief A minimalistic 32-bit PRNG tests suite for 16-bit computers;
 * it is compatible with ANSI C compilers.
 * @details This tests suite includes only 7 tests:
 *
 * - 1D 32-bit birthday spacings test (`bspace32_1d`)
 * - 4D 32-bit birthday spacings test (`bspace8_4d`)
 * - 8D 32-bit birthday spacings test (`bspace4_8d`)
 * - 8D 32-bit birthday spacings test with decimation (`bspace4_8d_dec`)
 * - Byte frequency test
 * - Linear complexity (lowest bit, `linearcomp:31`)
 * - Linear complexity (highest bit, `linearcomp:0`)
 *
 * Can be compiled by any ANSI C compiler for 16-bit, 32-bit and 64-bit
 * platforms. On modern computers (2025) it will run in less than 2 seconds,
 * in DosBOX 0.74 with 3000 cycles (~80286 16 MHz) it will run about 15-20
 * minutes.
 *
 * The next generators are implemented here:
 *
 * - `alfib`: x_{n} = {x_{n-55}} + {x_{n-24}} mod 2^{32}, fails 2 of 7.
 * - `lcg32`: x_{n+1} = 69069 x_{n} + 12345, fails 6 of 7.
 * - `lcg64`: x_{n+1} = 6906969069 x_{n} + 12345 [returns upper 32 bits],
 *   fails the `bspace4_8d_dec` test (1 of 7).
 * - `mwc1616`: a combination of two 16-bit MWC generators from KISS99.
 *   Fails the `bspace8_4d` test (1 of 7).
 * - `mwc1616x`: a combination of two 16-bit MWC generators that has a period
 *   about 2^{62}, passes all tests.
 * - `xorshift32`: fails 2 of 7, i.e. linear complexity tests.
 * - `xorwow`: an obsolete combined generator based on LFSR and "discrete Weyl
 *   sequence". Fails 2 of 7, i.e. the `bspace4_8d` and `linearcomp:0` tests.
 *
 * Compilation with Digital Mars C (from `apps` directory):
 *
 *     set path=%path%;c:\c_prog\dm\bin
 *     dmc sr_tiny.c -c -2 -o -mc -I../include -osr_tiny.obj
 *     dmc ../src/specfuncs.c -c -2 -o -mc -I../include -ospecfuncs.obj
 *     dmc sr_tiny.obj specfuncs.obj -mc -osr_tiny.exe
 *
 * Compilation with Open Watcom C (from `apps` directory): 
 *
 *     set path=c:\watcom\binnt;%PATH%
 *     set watcom=c:\watcom
 *     set include=c:\watcom\h
 *     wcc sr_tiny.c -mc -q -s -otexanhi -I..\include -fo=sr_tiny.obj
 *     wcc ..\src\specfuncs.c -mc -q -s -otexanhi -I..\include -fo=specfuncs.obj
 *     wcl sr_tiny.obj specfuncs.obj -q -fe=sr_tiny.exe
 *
 * WARNING!
 *
 * 1. In the case of compilation for x86-16 (16-bit systems such as DOS)
 *    "compact", "large" or "huge" memory models must be used. Otherwise "not
 *    enough memory" error will occur. Digital Mars and Open Watcom use
 *    the `-mc` command line key for that purpose.
 * 2. Usage of compilers that is not aware of LFNs (such as Borland Turbo C 2.0
 *    for DOS) will require a slight modification of source code to make it
 *    compatible with 8.3 file names convention.
 * 3. This file is designed for C89 (ANSI C)! Don't use features from C99 and
 *    later! It should be compilable with Turbo C 2.0, Digital Mars C,
 *    Open Watcom 11.0 and other ancient compilers!
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#define _STRICT_STDC
#include "smokerand/specfuncs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <float.h>


/*----------------------------------------------------------*/


typedef unsigned char u8;

#if UINT_MAX == 0xFFFFFFFF
typedef unsigned int u32;
typedef int i32;
#else
typedef unsigned long u32;
typedef long i32;
#endif

typedef unsigned short u16;

typedef u32 (*GetBits32Func)(void *state);

typedef struct {
    GetBits32Func get_bits32;
    void *state;
} Generator32State;

typedef struct TestResultEntry_ {
    char name[32];
    double x;
    double p;
    double alpha;
    struct TestResultEntry_ *next;
} TestResultEntry;


typedef struct {
    TestResultEntry *first;
    TestResultEntry *top;
} ResultsList;


void ResultsList_init(ResultsList *obj)
{
    obj->first = NULL;
    obj->top = NULL;
}

void ResultsList_add(ResultsList *obj, const TestResultEntry *entry)
{
    if (obj->first == NULL) {
        obj->first = malloc(sizeof(TestResultEntry));
        *(obj->first) = *entry;
        obj->first->next = NULL;
        obj->top = obj->first;
    } else {
        obj->top->next = malloc(sizeof(TestResultEntry));
        *(obj->top->next) = *entry;
        obj->top->next->next = NULL;
        obj->top = obj->top->next;
    }
}

static void sprintf_pvalue(char *buf, double p, double alpha)
{
    if (p != p || alpha != alpha) {
        sprintf(buf, "NAN");
    } else if (p < 0.0 || p > 1.0) {
        sprintf(buf, "???");
    } else if (p < DBL_MIN) {
        sprintf(buf, "0");
    } else if (1.0e-3 <= p && p <= 0.999) {
        sprintf(buf, "%.3f", p);
    } else if (p < 1.0e-3) {
        sprintf(buf, "%.2e", p);
    } else if (p > 0.999 && alpha > DBL_MIN) {
        sprintf(buf, "1 - %.2e", alpha);
    } else {
        sprintf(buf, "1");
    }
}



void ResultsList_add_poisson(ResultsList *obj,
    const char *name, double xemp, double mu)
{
    TestResultEntry tres;
    strcpy(tres.name, name);
    tres.x = xemp;
    tres.p = poisson_pvalue(tres.x, mu);
    tres.alpha = poisson_cdf(tres.x, mu);
    ResultsList_add(obj, &tres);
    printf("  %s: x = %g, p = %g\n", name, tres.x, tres.p);
}


void ResultsList_print(const ResultsList *obj)
{
    TestResultEntry *entry;
    int i, id = 1;
    printf("  %2s %-15s %12s %20s\n", "#", "Test name", "xemp", "p-value");
    for (i = 0; i < 60; i++) {
        printf("-");
    }
    printf("\n");
    for (entry = obj->first; entry != NULL; entry = entry->next) {
        char pbuf[32];
        sprintf_pvalue(pbuf, entry->p, entry->alpha);
        printf("  %2d %-15s %12g %20s\n", id++, entry->name, entry->x, pbuf);
    }
    for (i = 0; i < 60; i++) {
        printf("-");
    }
    printf("\n");
}


void ResultsList_free(ResultsList *obj)
{
    TestResultEntry *entry = obj->first;
    while (entry != NULL) {
        TestResultEntry *next = entry->next;
        free(entry);
        entry = next;
    }    
}


/*---------------------------------------------------------------
  ----- MWC1616 generator implementation (a part of KISS99) -----
  ---------------------------------------------------------------*/

/**
 * @brief MWC1616 state.
 * @details MWC1616X pseudorandom number generator is a combination of two
 * multiply-with-carry generators that use base 2^{16}. It is a part of KISS99
 * PRNG suggested by G.Marsaglia. It has poor quality and shouldn't be used
 * as a general purpose generator.
 */
typedef struct {
    u32 z;
    u32 w;
} Mwc1616State;

void Mwc1616State_init(Mwc1616State *obj, u32 seed)
{
    obj->z = (seed & 0xFFFF) | (1ul << 16ul);
    obj->w = (seed >> 16) | (1ul << 16ul);
}

u32 mwc1616_func(void *state)
{
    Mwc1616State *obj = state;
    obj->z = 36969ul * (obj->z & 0xFFFF) + (obj->z >> 16);
    obj->w = 18000ul * (obj->w & 0xFFFF) + (obj->w >> 16);
    return (obj->z << 16) + obj->w;
}


/*------------------------------------------------------------------------
  ----- MWC1616X generator implementation (a high-quality generator) -----
  ------------------------------------------------------------------------*/

/**
 * @brief MWC1616X state.
 * @details MWC1616X pseudorandom number generator is a combination of two
 * multiply-with-carry generators that use base 2^{16}. It has a period about
 * 2^{62} and good quality. It can be compiled for platforms without 64-bit
 * data types, e.g. for 16-bit x86 processors.
 *
 * Its quality is much better than quality of MWC1616.
 */
typedef Mwc1616State Mwc1616xState;


u32 mwc1616x_func(void *state)
{
    Mwc1616xState *obj = state;
    u16 z_lo = obj->z & 0xFFFF, z_hi = obj->z >> 16;
    u16 w_lo = obj->w & 0xFFFF, w_hi = obj->w >> 16;
    obj->z = 61578ul * z_lo + z_hi;
    obj->w = 63885ul * w_lo + w_hi;
    return ((obj->z << 16) | (obj->z >> 16)) ^ obj->w;
}

void Mwc1616xState_init(Mwc1616xState *obj, u32 seed)
{
    Mwc1616State_init(obj, seed);
}


/*--------------------------------------------------------------
  ----- Additive lagged Fibonacci generator implementation -----
  --------------------------------------------------------------*/

#define ALFIB_A 55
#define ALFIB_B 24

/**
 * @brief Additive lagged Fibonacci generator state.
 * @details This generator was popular several decades ago but fails 1D
 * 32-bit birthday spacings tests and its lowest bit has low linear
 * complexity. It also can cause problems with 
 */
typedef struct {
    u32 x[ALFIB_A];
    int i;
    int j;
} ALFibState;

static u32 alfib_func(void *state)
{
    ALFibState *obj = state;
    u32 x = obj->x[obj->i] + obj->x[obj->j];
    obj->x[obj->i] = x;
    if (++obj->i == ALFIB_A) obj->i = 0;
	if (++obj->j == ALFIB_A) obj->j = 0;
    return x;
}

/**
 * @brief Additive lagged Fibonacci initialization.
 * @details It uses MWC1616X generator because bias in Hamming weights
 * and linear complexity during initialization may preserve in output
 * and cause failures of byte frequency and linear complexity tests.
 */
static void ALFibState_init(ALFibState *obj, u32 seed)
{
    int i;
    Mwc1616xState mwc;
    Mwc1616xState_init(&mwc, seed);
    for (i = 0; i < ALFIB_A; i++) {
        obj->x[i] = mwc1616x_func(&mwc);
    }
    obj->i = 0;
    obj->j = ALFIB_A - ALFIB_B;
}


/*----------------------------------------------------------*/

u32 lcg69069func(void *state)
{
    u32 *x = (u32 *) state;
    *x = 69069 * (*x) + 12345;
    return *x;
}




/*---------------------------------------------------------------------
  ----- 64-bit LCG (linear congruentual) generator implementation -----
  ---------------------------------------------------------------------*/

#define HI32(x) ((x) >> 16)
#define LO32(x) ((x) & 0xFFFF)
#define MUL32(x,y) ((u32)(x) * (u32)(y))
#define SUM32(x,y) ((u32)(x) + (u32)(y))


/**
 * @brief A portable (compatible with ANSI C and 16-bit platforms)
 * version of 64-bit linear congruential generator.
 * @details Passes all tests except 8D 32-bit birthday spacings test
 * with decimation. Python 3.x program for testing:
 *
 *     x = 12345678
 *     for i in range(0, 10000):
 *         x = (6906969069 * x + 12345) % 2**64
 *     print(x >> 32)
 */
typedef struct {
    u16 x[4];
} Lcg64x16State;

void Lcg64x16State_init(Lcg64x16State *obj, u32 seed)
{
    obj->x[0] = seed & 0xFFFF;
    obj->x[1] = seed >> 16;
    obj->x[2] = 0;
    obj->x[3] = 0;
}


u32 lcg64x16_func(void *state)
{
    /* Made for multiplier 69069'69069 = 0x1'9BAF'FBED */
    /*                      lower   higher */
    static const u16 a[] = {0xFBED, 0x9BAF};
    static const u16 c = 12345;
    Lcg64x16State *obj = state;
    u16 row0[4], row1[3];
    u32 mul, sum, x0 = obj->x[0], x1 = obj->x[1];
    /* Row 0 */
    mul = MUL32(a[0], obj->x[0]); row0[0] = LO32(mul);
    mul = MUL32(a[0], obj->x[1]) + HI32(mul); row0[1] = LO32(mul);
    mul = MUL32(a[0], obj->x[2]) + HI32(mul); row0[2] = LO32(mul);
    mul = MUL32(a[0], obj->x[3]) + HI32(mul); row0[3] = LO32(mul);
    /* Row 1 */
    mul = MUL32(a[1], obj->x[0]); row1[0] = LO32(mul);
    mul = MUL32(a[1], obj->x[1]) + HI32(mul); row1[1] = LO32(mul);
    mul = MUL32(a[1], obj->x[2]) + HI32(mul); row1[2] = LO32(mul);
    /* Sum rows (update state) */
    sum = SUM32(row0[0], c);                   obj->x[0] = LO32(sum);
    sum = SUM32(row0[1], row1[0]) + HI32(sum); obj->x[1] = LO32(sum);
    sum = SUM32(row0[2], row1[1]) + HI32(sum); obj->x[2] = LO32(sum);
    sum = SUM32(row0[3], row1[2]) + HI32(sum); obj->x[3] = LO32(sum);
    /* Row 2 */
    sum = SUM32(obj->x[2], x0);             obj->x[2] = LO32(sum);
    sum = SUM32(obj->x[3], x1) + HI32(sum); obj->x[3] = LO32(sum);
    /* Return upper 32 bits */
    return ((u32) obj->x[3] << 16) | obj->x[2];

}

/*------------------------------------------------------
  ----- xorshift32 (shr3) generator implementation -----
  ------------------------------------------------------*/

u32 xorshift32_func(void *state)
{
    u32 *x = state;
    *x ^= (*x << 17);
    *x ^= (*x >> 13);
    *x ^= (*x << 5);
    return *x;
}

/*--------------------------------------------
  ----- xorwow  generator implementation -----
  --------------------------------------------*/

/**
 * @brief xorwow PRNG state.
 */
typedef struct {
    u32 x; /**< Xorshift register */
    u32 y; /**< Xorshift register */
    u32 z; /**< Xorshift register */
    u32 w; /**< Xorshift register */
    u32 v; /**< Xorshift register */
    u32 d; /**< "Weyl sequence" counter */
} XorWowState;


void XorWowState_init(XorWowState *obj, u32 seed)
{
    obj->x = 123456789;
    obj->y = 362436069;
    obj->z = 521288629;
    obj->w = 88675123;
    obj->v = ~seed;
    obj->d = seed;
}

u32 xorwow_func(void *state)
{
    static const u32 d_inc = 362437;
    XorWowState *obj = state;
    u32 t = (obj->x ^ (obj->x >> 2));
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v << 4)) ^ (t ^ (t << 1));
    return (obj->d += d_inc) + obj->v;
}


/*----------------------------------------------------------*/


/**
 * @brief Performs the a[i] ^= b[i] operation for the given vectors.
 * @param a    This vector (a) will be modified.
 * @param b    This vector (b) will not be modified.
 * @param len  Vectors lengths.
 */
static void xorbytes(u8 *a, const u8 *b, size_t len)
{
/*
    size_t i;
    for (i = 0; i < len; i++) {
        a[i] ^= b[i];
    }
*/

    u32 *av = (u32 *) a, *bv = (u32 *) b;
    size_t nwords = len / 4, i;
    for (i = 0; i < nwords; i++) {
        av[i] ^= bv[i];
    }
    a += nwords * 4;
    b += nwords * 4;
    for (i = 0; i < len % 4; i++) {
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
size_t berlekamp_massey(const u8 *s, size_t n)
{
    size_t L = 0; /* Complexity */
    size_t N = 0; /* Current position */
    size_t i;
    long m = -1;
    u8 *C = calloc(n, sizeof(u8)); /* Coeffs. */
    u8 *B = calloc(n, sizeof(u8)); /* Prev.coeffs. */
    u8 *T = calloc(n, sizeof(u8)); /* Temp. copy of coeffs. */
    if (C == NULL || B == NULL || T == NULL) {
        fprintf(stderr, "***** berlekamp_massey: not enough memory *****");
        free(C); free(B); free(T);
        exit(1);
    }
    C[0] = 1; B[0] = 1;
    while (N < n) {
        char d = s[N];
        for (i = 1; i <= L; i++) {
            d ^= C[i] & s[N - i];
        }
        if (d == 1) {
            memcpy(T, C, (L + 1) * sizeof(u8));
            xorbytes(&C[N - m], B, n - N + m);
            if (2*L <= N) {
                L = N + 1 - L;
                m = N;
                memcpy(B, T, (L + 1) * sizeof(u8));
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
 * @param nbits Number of bits.
 * @param bitpos Bit position (0 is the lowest).
 */
void linearcomp_test_c89(ResultsList *out, Generator32State *obj,
    size_t nbits, unsigned int bitpos)
{
    TestResultEntry tres;
    size_t i;
    double L, T;
    u8 *s = calloc(nbits, sizeof(u8));
    u32 mask = 1ul << bitpos;
    if (s == NULL) {
        fprintf(stderr, "***** linearcomp_test_89: not enough memory *****\n");
        exit(1);
    }
    printf("Linear complexity test\n");
    printf("  nbits: %ld\n", (long) nbits);
    for (i = 0; i < nbits; i++) {
        if (obj->get_bits32(obj->state) & mask)
            s[i] = 1;
    }
    L = berlekamp_massey(s, nbits);
    if (nbits & 1) {
        T = -L + (double) (nbits + 1) / 2.0;
    } else {
        T = L - (double) nbits / 2.0;
    }
    sprintf(tres.name, "linearcomp:%u", bitpos);
    tres.x = T;
    tres.p = linearcomp_Tcdf(T);
    tres.alpha = linearcomp_Tccdf(T);
    printf("  L = %g; T = %g; p = %g; 1 - p = %g\n\n",
        L, tres.x, tres.p, tres.alpha);
    ResultsList_add(out, &tres);
    free(s);
}



/*---------------------------------------------------------*/

void insertsort(u32 *x, int begin, int end)
{
    int i;
    for (i = begin + 1; i <= end; i++) {
        u32 xi = x[i];
        int j = i;
        while (j > begin && x[j - 1] > xi) {
            x[j] = x[j - 1];
            j--;
        }
        x[j] = xi;
    }
}

void quicksort(u32 *v, int begin, int end)
{
    int i = begin, j = end, med = (begin + end) / 2;
    u32 pivot = v[med];
    while (i <= j) {
        if (v[i] < pivot) {
            i++;
        } else if (v[j] > pivot) {
            j--;
        } else {
            u32 tmp = v[i];
            v[i++] = v[j];
            v[j--] = tmp;
        }
    }
    if (begin < j) {
        if (j - begin > 12) {
            quicksort(v, begin, j);
        } else {
            insertsort(v, begin, j);
        }
    }
    if (end > i) {
        if (end - i > 12) {
            quicksort(v, i, end);
        } else {
            insertsort(v, i, end);
        }
    }
}

/**
 * @brief Calculates number of duplicates (essentially xemp) for
 * birthday spacings test.
 */
int get_ndups(u32 *x, int n)
{
    int i, ndups = 0;
    quicksort(x, 0, n - 1);
    for (i = 0; i < n - 1; i++) {
        x[i] = x[i + 1] - x[i];
    }
    quicksort(x, 0, n - 2);
    for (i = 0; i < n - 2; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    return ndups;
}


double bytefreq_to_chi2emp(const u32 *bytefreq)
{
    int i;
    u32 Oi_sum = 0;
    double Ei, chi2emp = 0.0;
    for (i = 0; i < 256; i++) {
        Oi_sum += bytefreq[i];
    }
    Ei = Oi_sum / 256.0;
    for (i = 0; i < 256; i++) {
        double Oi = bytefreq[i];
        double d = Oi - Ei;
        chi2emp += d * d / Ei;
    }
    return chi2emp;
}

/**
 * @brief A buffer for birthday spacings test implementation.
 */
typedef struct {
    u32 *x; /**< Buffer for a sample */
    int len; /**< Number of values in the sample */
    int pos; /**< Current number of elements in the sample */
    int ndim; /**< Number of dimensions */
    unsigned long ndups; /**< Number of duplicates that was found */
    int nbits_per_dim; /**< Number of bits per dimensions */
    u32 mask; /**< Bit mask for extracting lower bits from values */
} BSpaceBuffer;


void BSpaceBuffer_init(BSpaceBuffer *obj, int len, int ndim)
{
    obj->x = calloc(len, sizeof(u32));
    if (obj->x == NULL) {
        fprintf(stderr, "***** Not enough memory *****\n");
        exit(1);
    }
    obj->len = len;
    obj->pos = 0;
    obj->ndups = 0;
    obj->ndim = ndim;
    obj->nbits_per_dim = 32 / ndim;
    obj->mask = (1ul << obj->nbits_per_dim) - 1;
}


void BSpaceBuffer_add_values(BSpaceBuffer *obj, const u32 *x_in)
{
    int i, j;
    int n = obj->len, ndim = obj->ndim, pos = obj->pos;
    int mask = obj->mask, nbits_per_dim = obj->nbits_per_dim;
    for (i = 0; i < n; i += ndim) {
        u32 tmp = 0;
        for (j = 0; j < ndim; j++) {
            tmp <<= nbits_per_dim;
            tmp |= x_in[i + j] & mask;
        }
        obj->x[pos++] = tmp;
    }
    if (pos >= n) {
        obj->ndups += get_ndups(obj->x, n);
        pos = 0;
    }
    obj->pos = pos;
}


void BSpaceBuffer_free(BSpaceBuffer *obj)
{
    free(obj->x);
}

/**
 * @brief Runs all statistical tests except linear complexity.
 * @param out  Pointer to an output structure for tests results.
 * @param obj  The pseudorandom number generator to be tested.
 */
void gen_tests(ResultsList *out, Generator32State *obj)
{
    const double lambda = 4.0;
    int n = 4096, i, ii, ndups = 0, nsamples = 1024;
    BSpaceBuffer bs_dec, bs_4x8d, bs_8x4d;
    double chi2emp = 0.0, mu = 0.0;
    TestResultEntry tres;
    u32 u_dec = 0;
    u32 *x = calloc(n, sizeof(u32));
    u32 *bytefreq = calloc(256, sizeof(u32));
    if (x == NULL || bytefreq == NULL) {
        fprintf(stderr, "***** Not enough memory *****\n");
        exit(1);
    }
    BSpaceBuffer_init(&bs_dec, n, 8);
    BSpaceBuffer_init(&bs_4x8d, n, 8);
    BSpaceBuffer_init(&bs_8x4d, n, 4);
    printf("Processing pseudorandom numbers blocks...\n");
    for (ii = 0; ii < nsamples; ii++) {
        for (i = 0; i < n; i++) {
            u32 u = obj->get_bits32(obj->state);
            x[i] = u;
            /* Making subsample for birthday spacings with decimation
               Take only every 64th point and use lower 4 bits from each.
               We will analyse 8-tuples made of 4-bit elements. */
            if ((i & 0x3F) == 0 && bs_dec.pos < n) {
                u_dec <<= 4;
                u_dec |= u & 0xF;
                /* Check if 8 elements are already collected */
                if ((i & 0x1C0) == 0x1C0) {
                    bs_dec.x[bs_dec.pos++] = u_dec;
                    u_dec = 0;
                }
            }
            /* Byte counting (with destruction of u value) */
            bytefreq[u & 0xFF]++; u >>= 8;
            bytefreq[u & 0xFF]++; u >>= 8;
            bytefreq[u & 0xFF]++; u >>= 8;
            bytefreq[u & 0xFF]++;            
        }
        /* Subsampling for nD birthday spacings without decimation */
        BSpaceBuffer_add_values(&bs_4x8d, x);        
        BSpaceBuffer_add_values(&bs_8x4d, x);
        /* 1D birthday spacings test */
        ndups += get_ndups(x, n);
        printf("  %d of %d\r", ii + 1, nsamples);
        fflush(stdout);
    }
    chi2emp = bytefreq_to_chi2emp(bytefreq);
    bs_dec.ndups = get_ndups(bs_dec.x, n);
    printf("\nBirthday spacings and byte frequency tests results\n");
    /* Analysis of birthday spacings tests */
    mu = nsamples * lambda;
    ResultsList_add_poisson(out, "bspace32_1d", ndups, mu);
    ResultsList_add_poisson(out, "bspace8_4d", bs_8x4d.ndups, mu / 4);
    ResultsList_add_poisson(out, "bspace4_8d", bs_4x8d.ndups, mu / 8);
    ResultsList_add_poisson(out, "bspace4_8d_dec", bs_dec.ndups, lambda);
    /* Analysis of byte frequency test */
    strcpy(tres.name, "bytefreq");
    tres.x = chi2emp;
    tres.p = chi2_pvalue(chi2emp, 255);
    tres.alpha = chi2_cdf(chi2emp, 255);
    ResultsList_add(out, &tres);
    printf("  bytefreq: x = %g, p = %g\n", tres.x, tres.p);
    /* Free all buffers */
    free(x);
    free(bytefreq);
    BSpaceBuffer_free(&bs_dec);
    BSpaceBuffer_free(&bs_4x8d);
    BSpaceBuffer_free(&bs_8x4d);
}

/**
 * @brief Prints a short command line arguments reference to stdout.
 */
void print_help()
{
    printf(
        "Test suite for 32-bit pseudorandom numbers generators.\n"
        "It is minimalistic and can be compiled even for 16-bit DOS.\n"
        "(C) 2024-2025 Alexey L. Voskov\n\n"
        "Usage: sr_tiny gen_name [speed]\n"
        "  gen_name = alfib, lcg32, lcg64, mwc1616x, xorshift32\n"
        "    alfib = LFib(55,24,+,2^32): additive lagged Fibonacci\n"
        "    lcg32 - 32-bit LCG; x_n = 69069x_{n-1} + 12345 mod 2^32\n"
        "    lcg64 - 64-bit LCG, returns upper 32 bits\n"
        "    mwc1616 - a combination of 2 MWC generators from KISS99,\n"
        "      an output has a relatively low quality\n"
        "    mwc1616x - a combination of 2 MWC generators, gives high\n"
        "      quality sequence that passes all tests\n"
        "    xorshift32 - classical 'shr3' LFSR PRNG\n"
        "    xorwow - an obsolete combined PRNG\n\n"
        "  speed - an optional argument, enables speed measurement mode.\n");
}

/**
 * @brief Creates a pseudorandom number generator (PRNG).
 * @param name Generator name, may be `alfib`, `lcg32`, `lcg64`, `mwc1616`,
 * `mwc1616x`, `xorshift32`, `xorwow`.
 */
Generator32State create_generator(const char *name)
{
    static Generator32State gen = {0, 0};
    u32 x = time(NULL);
    if (!strcmp("alfib", name)) {
        gen.state = malloc(sizeof(ALFibState));
        gen.get_bits32 = alfib_func;
        ALFibState_init(gen.state, x);
    } else if (!strcmp("lcg32", name)) {
        gen.state = malloc(sizeof(u32));
        gen.get_bits32 = lcg69069func;
        *((u32 *) gen.state) = x;
    } else if (!strcmp("lcg64", name)) {
        int i;
        u32 u, u_ref = 3675123773ul;
        gen.state = malloc(sizeof(Lcg64x16State));
        gen.get_bits32 = lcg64x16_func;
        Lcg64x16State_init(gen.state, 12345678);
        for (i = 0; i < 10000; i++) {
            u = lcg64x16_func(gen.state);
        }
        printf("LCG64 self-test: %lu (ref = %lu)\n",
            (unsigned long) u, (unsigned long) u_ref);
        if (u != u_ref) {
            free(gen.state);
            fprintf(stderr, "FAILED!\n");
            exit(1);
        }
        Lcg64x16State_init(gen.state, x);
    } else if (!strcmp("mwc1616", name)) {
        gen.state = malloc(sizeof(Mwc1616State));
        gen.get_bits32 = mwc1616_func;
        Mwc1616State_init(gen.state, x);
    } else if (!strcmp("mwc1616x", name)) {
        gen.state = malloc(sizeof(Mwc1616xState));
        gen.get_bits32 = mwc1616x_func;
        Mwc1616xState_init(gen.state, x);
    } else if (!strcmp("xorshift32", name)) {
        gen.state = malloc(sizeof(u32));
        gen.get_bits32 = xorshift32_func;
        *((u32 *) gen.state) = x | 0x1; /* Cannot be initialized with 0 */
    } else if (!strcmp("xorwow", name)) {
        gen.state = malloc(sizeof(XorWowState));
        XorWowState_init(gen.state, x);
        gen.get_bits32 = xorwow_func;
    }
    return gen;
}


/**
 * @brief Measures speed of the PRNG in KiB/sec.
 */
void measure_speed(Generator32State *gen)
{
    double nsec, kib_sec;
    long nblocks = 2, nblocks_total = 0;
    clock_t tic, toc;
    printf("Generator speed measurement\n");
    tic = clock();
    do {
        long i;
        int j;
        u32 sum = 0;
        for (i = 0; i < nblocks; i++) {
            for (j = 0; j < 16384; j++) { /* 64K block */
                sum += gen->get_bits32(gen->state);
            }
        }
        (void) sum;
        nblocks_total += nblocks;
        nblocks *= 2;
        toc = clock();
    } while (toc - tic < 2*CLOCKS_PER_SEC);
    nsec = ((double) (toc - tic) / CLOCKS_PER_SEC);
    kib_sec = (double) (nblocks_total << 6) / nsec;
    if (kib_sec < 1000.0) {
        printf("  Generator speed: %g KiB/sec", kib_sec);
    } else {
        printf("  Generator speed: %g MiB/sec", kib_sec / 1024);
    }
}

/**
 * @brief Prints the elapsed time in the hh:mm:ss.msec format to stdout.
 * @param sec_total  Elapsed time, seconds.
 */
void print_elapsed_time(double sec_total)
{
    unsigned long sec_total_int = sec_total;
    int ms = (sec_total - sec_total_int) * 1000.0;
    int seconds = sec_total_int % 60;
    int minutes = (sec_total_int / 60) % 60;
    int hours = (sec_total_int / 3600);
    printf("%.2d:%.2d:%.2d.%.3d\n", hours, minutes, seconds, ms);
}

/**
 * @brief Program entry point.
 */
int main(int argc, char *argv[])
{
    Generator32State gen;
    if (argc < 2) {
        print_help();
        return 0;
    }
    gen = create_generator(argv[1]);
    if (gen.state == NULL) {
        fprintf(stderr, "Unknown generator %s\n", argv[1]);
        return 1;
    }
    if (argc == 3 && !strcmp(argv[2], "speed")) {
        measure_speed(&gen);
    } else {
        ResultsList results;
        clock_t tic, toc;
        ResultsList_init(&results);
        tic = clock();
        gen_tests(&results, &gen);
        linearcomp_test_c89(&results, &gen, 10000, 31);
        linearcomp_test_c89(&results, &gen, 10000, 0);
        toc = clock();
        ResultsList_print(&results);
        ResultsList_free(&results);
        printf("Elaplsed time: ");
        print_elapsed_time(((double) (toc - tic) / CLOCKS_PER_SEC));
        printf("\n");
    }
    free(gen.state);
    return 0;
}
