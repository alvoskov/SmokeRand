/**
 * @file sr_tiny.c
 * @brief A minimalistic 32-bit PRNG tests suite for 16-bit computers;
 * it is compatible with ANSI C compilers.
 * @details This tests suite includes only 5 tests:
 *
 * - 1D 32-bit birthday spacings
 * - 8D 32-bit birthday spacings test with decimation
 * - Byte frequency test
 * - Linear complexity (lowest bit)
 * - Linear complexity (highest bit)
 *
 * Can be compiled by any ANSI C compiler for 16-bit, 32-bit and 64-bit
 * platforms. On modern computers (2024) it will run in less than 2 seconds,
 * in DosBOX 0.74 with 3000 cycles (~80286 16 MHz) it will run about 15-20
 * minutes.
 *
 * The next generators are implemented here:
 *
 * - alfib: x_{n} = {x_{n-55}} + {x_{n-24}} mod 2^{32}, fails 2 of 5.
 * - lcg32: x_{n+1} = 69069 x_{n} + 12345, fails 4 of 5.
 * - lcg64: x_{n+1} = 6906969069 x_{n} + 12345 [returns upper 32 bits], fails
 *   1 of 5.
 * - mwc1616x: a combination of two 16-bit MWC generators that has a period
 *   about 2^{62}, passes all tests.
 * - xorshift32: fails 2 of 5, i.e. linear complexity tests
 * 
 *
 * Compilation with Digital Mars C:
 *
 *     set path=%path%;c:\c_prog\dm\bin
 *     path
 *     dmc sr_tiny.c -c -2 -o -ms -I../include -osr_tiny.obj
 *     dmc specfuncs.c -c -2 -o -ms -I../include -ospecfuncs.obj
 *     dmc sr_tiny.obj specfuncs.obj -osr_tiny.exe
 *
 * WARNING! This file is designed for C89 (ANSI C)! Don't use features
 * from C99 and later! It should be compilable with Turbo C 2.0,
 * Digital Mars C, Open Watcom 11.0 and other ancient compilers!
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
typedef struct {
    u32 z;
    u32 w;
} Mwc1616xState;


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
    obj->z = (seed & 0xFFFF) | (1ul << 16ul);
    obj->w = (seed >> 16) | (1ul << 16ul);
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
void linearcomp_test(ResultsList *out, Generator32State *obj,
    size_t nbits, unsigned int bitpos)
{
    TestResultEntry tres;
    size_t i;
    double parity, mu, sigma, L;
    u8 *s = calloc(nbits, sizeof(u8));
    u32 mask = 1ul << bitpos;
    if (s == NULL) {
        fprintf(stderr, "***** linearcomp_test: not enough memory *****\n");
        exit(1);
    }
    printf("Linear complexity test\n");
    printf("  nbits: %ld\n", (long) nbits);
    for (i = 0; i < nbits; i++) {
        if (obj->get_bits32(obj->state) & mask)
            s[i] = 1;
    }
    parity = nbits & 1;
    mu = nbits / 2.0 + (9.0 - parity) / 36.0;
    sigma = sqrt(86.0 / 81.0);
    L = berlekamp_massey(s, nbits);
    sprintf(tres.name, "linearcomp:%u", bitpos);
    tres.x = (L - mu) / sigma;
    tres.p = stdnorm_pvalue(tres.x);
    tres.alpha = stdnorm_cdf(tres.x);
    printf("  L = %g; z = %g; p = %g; 1 - p = %g\n\n",
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

void gen_tests(ResultsList *out, Generator32State *obj)
{
    const double lambda = 4.0;
    int n = 4096, i, ii, ndups = 0, ndups_dec, nsamples = 1024;
    int pos_dec = 0;
    double chi2emp = 0.0;
    TestResultEntry tres;
    u32 u_dec = 0;
    u32 *x = calloc(n, sizeof(u32));
    u32 *x_dec = calloc(n, sizeof(u32));
    u32 *bytefreq = calloc(256, sizeof(u32));
    printf("Processing pseudorandom numbers blocks...\n");
    for (ii = 0; ii < nsamples; ii++) {
        for (i = 0; i < n; i++) {
            u32 u = obj->get_bits32(obj->state);
            x[i] = u;
            /* Making subsample for birthday spacings with decimation
               Take only every 64th point and use lower 4 bits from each.
               We will analyse 8-tuples made of 4-bit elements. */
            if ((i & 0x3F) == 0 && pos_dec < n) {
                u_dec <<= 4;
                u_dec |= u & 0xF;
                /* Check if 8 elements are already collected */
                if ((i & 0x1C0) == 0x1C0) {
                    x_dec[pos_dec++] = u_dec;
                    u_dec = 0;
                }
            }
            /* Byte counting (with destruction of u value) */
            bytefreq[u & 0xFF]++; u >>= 8;
            bytefreq[u & 0xFF]++; u >>= 8;
            bytefreq[u & 0xFF]++; u >>= 8;
            bytefreq[u & 0xFF]++;            
        }
        ndups += get_ndups(x, n);
        printf("  %d of %d\r", ii + 1, nsamples);
    }
    chi2emp = bytefreq_to_chi2emp(bytefreq);
    ndups_dec = get_ndups(x_dec, n);
    printf("\nBirthday spacings and byte frequency tests results\n");
    /* Analysis of 1-dimensional birthday spacings test */
    strcpy(tres.name, "bspace32_1d");
    tres.x = ndups;
    tres.p = poisson_pvalue(tres.x, nsamples * lambda);
    tres.alpha = poisson_cdf(tres.x, nsamples * lambda);
    ResultsList_add(out, &tres);
    printf("  bspace32_1d: x = %g, p = %g\n", tres.x, tres.p);
    /* Analysis of 3D birthday spacings test with decimation */
    strcpy(tres.name, "bspace4_8_dec");
    tres.x = ndups_dec;
    tres.p = poisson_pvalue(tres.x, lambda);
    tres.alpha = poisson_cdf(tres.x, lambda);
    ResultsList_add(out, &tres);
    printf("  bspace4_8d_dec: x = %g, p = %g\n", tres.x, tres.p);
    /* Analysis of byte frequency test */
    strcpy(tres.name, "bytefreq");
    tres.x = chi2emp;
    tres.p = chi2_pvalue(chi2emp, 255);
    tres.alpha = chi2_cdf(chi2emp, 255);
    ResultsList_add(out, &tres);
    printf("  bytefreq: x = %g, p = %g\n", tres.x, tres.p);
    /* Free all buffers */
    free(x);
    free(x_dec);
    free(bytefreq);
}


void print_help()
{
    printf("Test suite for 32-bit pseudorandom numbers generators.\n");
    printf("It is minimalistic and can be compiled even for 16-bit DOS.\n");
    printf("(C) 2024 Alexey L. Voskov\n\n");
    printf("Usage: sr_tiny gen_name [speed]\n");
    printf("  gen_name = alfib, lcg32, lcg64, mwc1616x, xorshift32\n");
    printf("    alfib = LFib(55,24,+,2^32): additive lagged Fibonacci\n");
    printf("    lcg32 - 32-bit LCG; x_n = 69069x_{n-1} + 12345 mod 2^32\n");
    printf("    lcg64 - 64-bit LCG, returns upper 32 bits\n");
    printf("    mwc1616x - a combination of 2 MWC generators, gives high\n");
    printf("      quality sequence that passes all tests\n");
    printf("    xorshift32 - classical 'shr3' LFSR PRNG\n\n");
    printf("  speed - optional argument, enables speed measurement mode.\n");
}


Generator32State create_generator(const char *name)
{
    static Generator32State gen = {NULL, NULL};
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
    } else if (!strcmp("mwc1616x", name)) {
        gen.state = malloc(sizeof(Mwc1616xState));
        gen.get_bits32 = mwc1616x_func;
        Mwc1616xState_init(gen.state, x);
    } else if (!strcmp("xorshift32", name)) {
        gen.state = malloc(sizeof(u32));
        gen.get_bits32 = xorshift32_func;
        *((u32 *) gen.state) = x | 0x1; /* Cannot be initialized with 0 */
    }
    return gen;
}



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

void print_elapsed_time(double sec_total)
{
    unsigned long sec_total_int = sec_total;
    int ms = (sec_total - sec_total_int) * 1000.0;
    int seconds = sec_total_int % 60;
    int minutes = (sec_total_int / 60) % 60;
    int hours = (sec_total_int / 3600);
    printf("%.2d:%.2d:%.2d.%.3d\n", hours, minutes, seconds, ms);
}

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
        linearcomp_test(&results, &gen, 10000, 31);
        linearcomp_test(&results, &gen, 10000, 0);
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
