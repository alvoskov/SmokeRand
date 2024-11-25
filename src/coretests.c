/**
 * @file coretests.c
 * @brief The key tests for testings PRNGs: frequency tests, Marsaglia's
 * birthday spacings and monkey tests, Knuth's gap test. Also includes
 * Hamming weights based DC6 test from PractRand.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/coretests.h"
#include "smokerand/specfuncs.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/////////////////////////////////////////////////
///// Birthday spacings test implementation /////
/////////////////////////////////////////////////


/**
 * @brief Make non-overlapping 64-bit tuples (points in n-dimensional space)
 * for birthday spacings test. It may use either higher or lower bits.
 */
static inline void bspace_make_tuples64(const BSpaceNDOptions *opts,
    const GeneratorInfo *gi, void *state, uint64_t *u, size_t len)
{
    if (opts->get_lower) {
        // Take lower bits
        uint64_t mask;
        if (opts->nbits_per_dim == 64) {
            mask = 0xFFFFFFFFFFFFFFFF;
        } else {
            mask = (1ull << opts->nbits_per_dim) - 1ull;
        }
        for (size_t j = 0; j < len; j++) {
            u[j] = 0;
            for (size_t k = 0; k < opts->ndims; k++) {
                u[j] <<= opts->nbits_per_dim;
                u[j] |= gi->get_bits(state) & mask;
            }
        }
    } else {
        // Take higher bits
        unsigned int shl = gi->nbits - opts->nbits_per_dim;
        for (size_t j = 0; j < len; j++) {
            u[j] = 0;
            for (size_t k = 0; k < opts->ndims; k++) {
                u[j] <<= opts->nbits_per_dim;
                u[j] |= gi->get_bits(state) >> shl;
            }
        }
    }
}


/**
 * @brief Make non-overlapping 32-bit tuples (points in n-dimensional space)
 * for birthday spacings test. It may use either higher or lower bits.
 */
static inline void bspace_make_tuples32(const BSpaceNDOptions *opts,
    const GeneratorInfo *gi, void *state, uint32_t *u, size_t len)
{
    if (opts->get_lower) {
        // Take lower bits
        uint64_t mask;
        if (opts->nbits_per_dim == 32) {
            mask = 0xFFFFFFFF;
        } else {
            mask = (1ull << opts->nbits_per_dim) - 1ull;
        }
        for (size_t j = 0; j < len; j++) {
            u[j] = 0;
            for (size_t k = 0; k < opts->ndims; k++) {
                u[j] <<= opts->nbits_per_dim;
                u[j] |= gi->get_bits(state) & mask;
            }
        }
    } else {
        // Take higher bits
        unsigned int shl = gi->nbits - opts->nbits_per_dim;
        for (size_t j = 0; j < len; j++) {
            u[j] = 0;
            for (size_t k = 0; k < opts->ndims; k++) {
                u[j] <<= opts->nbits_per_dim;
                u[j] |= gi->get_bits(state) >> shl;
            }
        }
    }
}


static unsigned int bspace_get_ndups64(uint64_t *x, size_t len)
{
    unsigned int ndups = 0;
    radixsort64(x, len);
    for (size_t i = 0; i < len - 1; i++) {
        x[i] = x[i + 1] - x[i];
    }
    radixsort64(x, len - 1);
    for (size_t i = 0; i < len - 2; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    return ndups;
}


static unsigned int bspace_get_ndups32(uint32_t *x, size_t len)
{
    unsigned int ndups = 0;
    radixsort32(x, len);
    for (size_t i = 0; i < len - 1; i++) {
        x[i] = x[i + 1] - x[i];
    }
    radixsort32(x, len - 1);
    for (size_t i = 0; i < len - 2; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    return ndups;
}

static unsigned long bspace_calc_len(unsigned int nbits_total)
{
    return (unsigned long) pow(2.0, (nbits_total + 4.0) / 3.0);
}

static double bspace_calc_lambda(size_t len, unsigned int nbits_total)
{
    return pow((double) len, 3.0) / (4 * pow(2.0, nbits_total));
}

/**
 * @brief 32-bit version of n-dimensional birthday spacings test.
 * @return Number of duplicates.
 */
static unsigned long bspace32_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    size_t len = bspace_calc_len(nbits_total);
    uint32_t *u = calloc(len, sizeof(uint32_t));
    if (u == NULL) {
        fprintf(stderr, "***** bspace32_nd_test: not enough memory *****\n");
        exit(1);
    }
    unsigned long *ndups = calloc(opts->nsamples, sizeof(unsigned long));
    if (ndups == NULL) {
        fprintf(stderr, "***** bspace32_nd_test: not enough memory *****\n");
        exit(1);
    }
    for (size_t i = 0; i < opts->nsamples; i++) {
        bspace_make_tuples32(opts, obj->gi, obj->state, u, len);
            ndups[i] = bspace_get_ndups32(u, len);
    }
    unsigned long ndups_total = 0;
    for (size_t i = 0; i < opts->nsamples; i++) {
        ndups_total += ndups[i];
    }
    free(u);
    free(ndups);
    return ndups_total;
}

/**
 * @brief 64-bit version of n-dimensional birthday spacings test.
 * @return Number of duplicates.
 */
static unsigned long bspace64_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    size_t len = bspace_calc_len(nbits_total);
    uint64_t *u = calloc(len, sizeof(uint64_t));
    if (u == NULL) {
        fprintf(stderr, "***** bspace64_nd_test: not enough memory *****\n");
        exit(1);
    }
    unsigned long *ndups = calloc(opts->nsamples, sizeof(unsigned long));
    if (ndups == NULL) {
        fprintf(stderr, "***** bspace64_nd_test: not enough memory *****\n");
        exit(1);
    }
    for (size_t i = 0; i < opts->nsamples; i++) {
        bspace_make_tuples64(opts, obj->gi, obj->state, u, len);
            ndups[i] = bspace_get_ndups64(u, len);
    }
    unsigned long ndups_total = 0;
    for (size_t i = 0; i < opts->nsamples; i++) {
        ndups_total += ndups[i];
    }
    free(u);
    free(ndups);
    return ndups_total;
}



/**
 * @brief n-dimensional birthday spacings test.
 * @details An implementation of classical N-dimensional birthday spacings test
 * suggested by G. Marsaglia. The number of duplicates in spacings obeys
 * Poisson distribution with the next mathematical expectance:
 *
 * \f[
 * \lambda = \frac{n^3}{4m}
 * \f]
 *
 * where \f$n\f$ is the number of points in the sample and \f$m\f$ is the
 * number of "birthdays" (number of possible values per point, if each point
 * is encoded by \f$ k \f$ bits - then \f$ m = 2^k \f$.
 *
 * The \f$n\f$ value is automatically selected to provide \f$ \lambda = 4\f$,
 * an optimal parameter for Poisson distribution.
 *
 * In the case of 1-dimensional test one point is made from 1 pseudorandom
 * value, in the case of N-dimensional test -- from lower or higher bits of 
 * N points. These tuples are NOT OVERLAPPING!
 *
 * References:
 *
 * 1. Marsaglia G., Tsang W. W. Some Difficult-to-pass Tests of Randomness
 *    // Journal of Statistical Software. 2002. V. 7. N. 3. P.1-9.
 *    https://doi.org/10.18637/jss.v007.i03
 */
TestResults bspace_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    TestResults ans = TestResults_create("bspace_nd");
    if (opts->ndims * opts->nbits_per_dim > 64 ||
        (obj->gi->nbits != 32 && obj->gi->nbits != 64)) {
        return ans;
    }
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    unsigned long len = bspace_calc_len(nbits_total);
    double lambda = bspace_calc_lambda(len, nbits_total);
    // Show information about the test
    obj->intf->printf("Birthday spacings test\n");
    obj->intf->printf("  ndims = %u; nbits_per_dim = %u; get_lower = %d\n",
        opts->ndims, opts->nbits_per_dim, opts->get_lower);
    obj->intf->printf("  nsamples = %lu; len = %lu, lambda = %g\n",
        opts->nsamples, len, lambda);
    // Compute number of duplicates
    unsigned long ndups_total = 0;
    if (nbits_total > 32) {
        ndups_total = bspace64_nd_test(obj, opts);
    } else {
        ndups_total = bspace32_nd_test(obj, opts);
    }
    ans.x = (double) ndups_total;
    ans.p = poisson_pvalue(ans.x, lambda * opts->nsamples);
    ans.alpha = poisson_cdf(ans.x, lambda * opts->nsamples);
    obj->intf->printf("  x = %.0f; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    return ans;
}

static uint64_t get_bits64_from32(void *state)
{
    GeneratorState *obj = state;
    uint64_t x = obj->gi->get_bits(obj->state);
    uint64_t y = obj->gi->get_bits(obj->state);
    return (x << 32) | y;
}


/**
 * @brief One-dimensional 32-bit birthday spacings test.
 * In the case of 32-bit PRNG it automatically switches to the 2D 32-bit
 * birthday spacings test.
 */
TestResults bspace64_1d_ns_test(GeneratorState *obj, unsigned int nsamples)
{
    BSpaceNDOptions opts;
    opts.nbits_per_dim = 64;
    opts.ndims = 1;
    opts.nsamples = nsamples;
    opts.get_lower = 1;
    if (obj->gi->nbits == 64) {
        return bspace_nd_test(obj, &opts);
    } else {
        GeneratorInfo genenv_info = *(obj->gi);
        GeneratorState genenv;
        genenv_info.get_bits = get_bits64_from32;
        genenv.gi = &genenv_info;
        genenv.state = obj; // Enveloped generator
        genenv.intf = obj->intf;
        return bspace_nd_test(&genenv, &opts);
    }
}


/**
 * @brief A specialized version of birthday spacings test designed for
 * detection of truncated 128-bit and 96-bit LCGs with modulo \f$ 2^k \f$.
 * @details It is a modification of 8-dimensional 8-bit (lower bits) birthday
 * spacings test. But only every 1 of `step` values are used for construction
 * of tuples. The `step` value should be power of 2. Such decimation reduces
 * an effective period of LCG and allows to detect anomalies in 96-bit
 * and 128-bit LCGs with module \f$ 2^{96} \f$ or \f$ 2^{128} \f$. Not
 * sensitive to LCGs with prime modulus.
 *
 * An idea of the test was partially inspired by the TMFn test from
 * PractRand 0.94 test suite by Chris Doty-Humphrey. The TMFn test also uses
 * decimation and takes only 1 or 2 64-bit values at the beginning of blocks
 * consisting of 4096 bytes. But they are analyzed by some mysterous bitwise
 * operations and g-test, not by means of birthday spacings.
 *
 * Sensitivity of 64-bit version of decimated birthday spacings test
 * for different steps:
 *
 * - 256 is enough to detect 128-bit LCG with truncation of lower 64 bits
 * - 8192 is enough to detect the same PRNG with truncation of lower 96 bits.
 *
 * Sensitivity of 32-bit version for different steps:
 *
 * - 64 is enough to detect 64-bit LCG with truncation of lower 32 bits
 * - 4096 is enough to detect 128-bit LCG with truncation of lower 64 bits
 * - 262144 is enough to detect the same PRNG with truncation of lower 96 bits.
 *
 * But 64-bit version (8_8d) requires 6658042 points instead of 4096, i.e.
 * 1626 times more. And transition to 32-bit version gives about 20x
 * performance boost and allows to detect 128-bit LCGs even in fast batteries.
 *
 * 32-bit decimated birthday spacings test is much more sensitive than TMFn
 * from PractRand: it requires 1 GiB to detect 128-bit LCG with truncated 64 bits
 * (PractRand needs 128 GiB) and 32 GiB to detect 128-bit LCG with truncated 96 bits
 * (PractRand doesn't detect it at 32 TiB sample).
 *
 * @param obj   Analyzed generator and its state.
 * @param step  Decimation step: only 1 of `step` values will be used by the
 *              test. 4096 is enough to detect 128-bit LCG with truncation of
 *              lower 64 bits, 262144 is enough to detect the same PRNG with
 *              truncation of lower 96 bits.
 */
TestResults bspace4_8d_decimated_test(GeneratorState *obj, unsigned int step)
{
    TestResults ans = TestResults_create("bspace4_8d");
    const unsigned int nbits_total = 32;
    size_t len = bspace_calc_len(nbits_total);
    double lambda = bspace_calc_lambda(len, nbits_total);
    // Show information about the test
    obj->intf->printf("Birthday spacings test with decimation\n");
    obj->intf->printf("  ndims = 8; nbits_per_dim = 4; step = %u\n", step);
    obj->intf->printf("  nsamples = 1; len = %lld, lambda = %g\n",
        (unsigned long long) len, lambda);
    // Run the test
    uint32_t *u = calloc(len, sizeof(uint32_t));
    uint32_t *u_high = calloc(len, sizeof(uint32_t));
    if (u == NULL || u_high == NULL) {
        fprintf(stderr, "***** bspace4_8d_decimated: not enough memory *****\n");
        free(u); free(u_high);
        exit(1);
    }

    for (size_t i = 0; i < len; i++) {
        for (int j = 0; j < 8; j++) {
            uint64_t x = obj->gi->get_bits(obj->state);
            // Take lower 4 bits
            u[i] <<= 4;
            u[i] |=  x & 0xF;
            // Take higher 4 bits
            u_high[i] <<= 4;
            u_high[i] |= reverse_bits4(x >> (obj->gi->nbits - 4));
            // Decimation
            for (unsigned int k = 0; k < step - 1; k++) {
                (void) obj->gi->get_bits(obj->state);
            }
        }
    }
    // Compute number of duplicates and p-values for lower bits
    ans.x = (double) bspace_get_ndups32(u, len);
    ans.p = poisson_pvalue(ans.x, lambda);
    ans.alpha = poisson_cdf(ans.x, lambda);
    obj->intf->printf("  Lower bits: x = %.0f; p = %g\n", ans.x, ans.p);
    // Compute the same values for higher bits
    double x_high = (double) bspace_get_ndups32(u_high, len);
    double p_high = poisson_pvalue(x_high, lambda);
    double alpha_high = poisson_cdf(x_high, lambda);
    obj->intf->printf("  Higher bits: x = %.0f; p = %g\n", x_high, p_high);
    // Select the worst p-value
    if (p_high < 1e-10 && p_high < ans.p) {
        obj->intf->printf("  Problems with higher bits are detected\n");
        ans.x = x_high;
        ans.p = p_high;
        ans.alpha = alpha_high;
    }
    obj->intf->printf("  x = %.0f; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    free(u);
    free(u_high);
    return ans;
}

/////////////////////////////////////////////
///// CollisionOver test implementation /////
/////////////////////////////////////////////

/**
 * @brief Make overlapping tuples (points in n-dimensional space) for
 * collisionover test. It may use either higher or lower bits.
 */
static inline void collisionover_make_tuples(const BSpaceNDOptions *opts,
    const GeneratorInfo *gi, void *state, uint64_t *u, size_t len)
{
    uint64_t cur_tuple = 0;
    const int rshift = (opts->ndims - 1) * opts->nbits_per_dim;
    if (opts->get_lower) {
        // Take lower bits
        uint64_t mask;
        if (opts->nbits_per_dim == 64) {
            mask = 0xFFFFFFFFFFFFFFFF;
        } else {
            mask = (1ull << opts->nbits_per_dim) - 1ull;
        }
        // Initialize the first tuple
        for (int j = 0; j < 8; j++) {
            cur_tuple >>= opts->nbits_per_dim;
            cur_tuple |= (gi->get_bits(state) & mask) << rshift;
        }
        // Generate other tuples
        for (size_t i = 0; i < len; i++) {
            cur_tuple >>= opts->nbits_per_dim;
            cur_tuple |= (gi->get_bits(state) & mask) << rshift;
            u[i] = cur_tuple;
        }
    } else {
        // Take higher bits
        unsigned int shr = gi->nbits - opts->nbits_per_dim;
        // Initialize the first tuple
        for (int j = 0; j < 8; j++) {
            cur_tuple >>= opts->nbits_per_dim;
            cur_tuple |= (gi->get_bits(state) >> shr) << rshift;
        }
        // Generate other tuples
        for (size_t i = 0; i < len; i++) {
            cur_tuple >>= opts->nbits_per_dim;
            cur_tuple |= (gi->get_bits(state) >> shr) << rshift;
            u[i] = cur_tuple;
        }
    }
}


/**
 * @brief "CollisionOver" modification of "Monkey test". The algorithm was
 * suggested by developers of TestU01.
 * @details Number of collision obeys the Poisson distribution with the next
 * mathematical expectance:
 *
 * \f[
 * \mu = d^t \left(\lambda - 1 + e^{-\lambda} \right)
 * \f]
 *
 * where \f$d\f$ is number of states per dimensions, \f$t\f$ is the number
 * of dimensions and \f$\lambda\f$ is so-called "density":
 *
 * \f[
 * \lambda = \frac{n - t + 1}{d^t}
 * \f]
 *
 * where \f$n\f$ is the number of points in the sample. The formula are correct
 * only when \f$ n \ll d^t \f$.
 *
 * The formula are taken from the TestU01 user manual.
 */
TestResults collisionover_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    size_t n = 50000000;
    uint64_t *u = calloc(n, sizeof(uint64_t));
    if (u == NULL) {
        fprintf(stderr, "***** collisionover_test: not enough memory *****\n");
        exit(1);
    }
    uint64_t nstates_u64 = 1ull << opts->ndims * opts->nbits_per_dim;
    uint64_t Oi[4] = {0, 0, 0, 0};
    Oi[0] = nstates_u64;
    double nstates = nstates_u64;
    double lambda = (n - opts->ndims + 1.0) / nstates;
    double mu = nstates * (lambda - 1 + exp(-lambda));
    TestResults ans;
    ans.name = "CollisionOver";
    obj->intf->printf("CollisionOver test\n");
    obj->intf->printf("  ndims = %d; nbits_per_dim = %d; get_lower = %d\n",
        opts->ndims, opts->nbits_per_dim, opts->get_lower);
    obj->intf->printf("  nsamples = %lu; len = %lu, mu = %g * %d\n",
        opts->nsamples, n, mu, (int) opts->nsamples);

    ans.x = 0;
    for (unsigned long i = 0; i < opts->nsamples; i++) {
        collisionover_make_tuples(opts, obj->gi, obj->state, u, n);
        // Find collisions by sorting the array
        radixsort64(u, n);
        size_t ncopies = 0;
        for (size_t i = 0; i < n - 1; i++) {
            if (u[i] == u[i + 1]) {
                ncopies++;
            } else {
                Oi[(ncopies < 3) ? (ncopies + 1) : 3]++;
                Oi[0]--;
                ncopies = 0;
            }
        }
    }
    ans.x += Oi[2];
    ans.p = poisson_pvalue(ans.x, mu * opts->nsamples);
    ans.alpha = poisson_cdf(ans.x, mu * opts->nsamples);
    // Frequency table
    double Ei = exp(-lambda) * nstates;
    obj->intf->printf("  Frequencies table (average per sample)\n");
    obj->intf->printf(" -----------------------------------------\n");
    obj->intf->printf("  %5s %16s %16s\n", "Freq", "Oi", "Ei");
    for (int i = 0; i < 4; i++) {
        uint64_t Oi_normed = (i == 0) ? Oi[i] : Oi[i] / opts->nsamples;
        obj->intf->printf("  %5d %16lld %16.1f\n", i, Oi_normed, Ei);
        Ei *= lambda / (i + 1.0);
    }
    obj->intf->printf(" -----------------------------------------\n");
    // Calculate statistics for p-value
    obj->intf->printf("  lambda = %g, mu = %g * %d\n",
        lambda, mu, (int) opts->nsamples);
    obj->intf->printf("  x = %g; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    free(u);
    return ans;
}

///////////////////////////////////
///// Gap test implementation /////
///////////////////////////////////

static int gap_test_guard(GeneratorState *obj, const GapOptions *opts)
{
    int is_ok = 0;
    uint64_t beta = 1ull << (obj->gi->nbits - opts->shl);
    double p = 1.0 / (1 << opts->shl); // beta in the floating point format
    unsigned long long nsamples = -20.0 / log10(1.0 - p);
    for (unsigned long i = 0; i < nsamples; i++) {
        if (obj->gi->get_bits(obj->state) <= beta) {
            is_ok = 1;
            break;
        }
    }
    return is_ok;
}

/**
 * @brief Knuth's gap test for detecting lagged Fibonacci generators.
 * @details Gap is \f$ [0;\beta) \f$ where $\beta = 1 / (2^{shl}) \f$.
 */
TestResults gap_test(GeneratorState *obj, const GapOptions *opts)
{
    const double Ei_min = 10.0;
    double p = 1.0 / (1 << opts->shl); // beta in the floating point format
    uint64_t beta = 1ull << (obj->gi->nbits - opts->shl);
    uint64_t u;
    size_t ngaps = opts->ngaps;
    size_t nbins = log(Ei_min / (ngaps * p)) / log(1 - p);
    size_t *Oi = calloc(nbins + 1, sizeof(size_t));
    if (Oi == NULL) {
        fprintf(stderr, "***** gap_test: not enough memory *****\n");
        exit(1);
    }
    unsigned long long nvalues = 0;
    TestResults ans = TestResults_create("Gap");
    obj->intf->printf("Gap test\n");
    obj->intf->printf("  alpha = 0.0; beta = %g; shl = %u;\n", p, opts->shl);
    obj->intf->printf("  ngaps = %lu; nbins = %lu\n",
        (unsigned long) ngaps, (unsigned long) nbins);
    if (!gap_test_guard(obj, opts)) {
        obj->intf->printf("  Generator output doesn't hit the gap! p <= 1e-15\n");
        ans.p = 1.0e-15;
        ans.alpha = 1.0 - ans.p;
        ans.x = NAN;
        return ans;
    }
    for (size_t i = 0; i < ngaps; i++) {
        size_t gap_len = 0;
        u = obj->gi->get_bits(obj->state);
        nvalues++;
        while (u > beta) {
            gap_len++;
            u = obj->gi->get_bits(obj->state);
            nvalues++;
        }
        Oi[(gap_len < nbins) ? gap_len : nbins]++;
    }
    ans.x = 0.0; // chi2emp
    for (size_t i = 0; i < nbins; i++) {
        double Ei = p * pow(1.0 - p, i) * ngaps;
        double d = Ei - Oi[i];
        ans.x += d * d / Ei;
    }
    free(Oi);
    ans.p = chi2_pvalue(ans.x, nbins - 1);
    ans.alpha = chi2_cdf(ans.x, nbins - 1);
    obj->intf->printf("  Values processed: %llu (2^%.1f)\n",
        nvalues, sr_log2(nvalues));
    obj->intf->printf("  x = %g; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    return ans;
}

//////////////////////////////////////////
///// Frequency tests implementation /////
//////////////////////////////////////////

/**
 * @brief Monobit frequency test.
 */
TestResults monobit_freq_test(GeneratorState *obj)
{
    int sum_per_byte[256];
    unsigned long long len = 1ul << 28;
    TestResults ans;
    ans.name = "MonobitFreq";
    for (size_t i = 0; i < 256; i++) {
        uint8_t u = i;
        sum_per_byte[i] = 0;
        for (size_t j = 0; j < 8; j++) {
            if ((u & 0x1) != 0) {
                sum_per_byte[i]++;
            }
            u >>= 1;
        }
        sum_per_byte[i] -= 4;
    }
    int64_t bitsum = 0;
    unsigned int nbytes = obj->gi->nbits / 8;
    for (unsigned long long i = 0; i < len; i++) {
        uint64_t u = obj->gi->get_bits(obj->state);
        for (unsigned int j = 0; j < nbytes; j++) {
            bitsum += sum_per_byte[u & 0xFF];
            u >>= 8;
        }
    }
    ans.x = fabs((double) bitsum) / sqrt(len * obj->gi->nbits);
    ans.p = stdnorm_pvalue(ans.x);
    ans.alpha = stdnorm_cdf(ans.x);
    obj->intf->printf("Monobit frequency test\n");
    obj->intf->printf("  Number of bits: %llu\n", len * obj->gi->nbits);
    obj->intf->printf("  sum = %lld; x = %g; p = %g\n",
        bitsum, ans.x, ans.p);
    obj->intf->printf("\n");    
    return ans;
}


static int cmp_doubles(const void *aptr, const void *bptr)
{
    double aval = *((double *) aptr), bval = *((double *) bptr);
    if (aval < bval) return -1;
    else if (aval == bval) return 0;
    else return 1;
}


/**
 * @brief Frequencies of n-bit words.
 */
TestResults nbit_words_freq_test(GeneratorState *obj,
    const unsigned int bits_per_word, const unsigned int average_freq)
{
    size_t block_len = (1ull << (bits_per_word)) * average_freq;
    size_t nblocks = 4096;
    unsigned int nwords_per_num = obj->gi->nbits / bits_per_word;
    size_t nwords = nwords_per_num * block_len;
    size_t nbins = 1ull << bits_per_word;
    uint64_t mask = nbins - 1;
    double *chi2 = calloc(nblocks, sizeof(double));
    size_t *wfreq = calloc(nwords, sizeof(size_t));
    
    TestResults ans;
    if (bits_per_word == 8) {
        ans.name = "BytesFreq";
        obj->intf->printf("Byte frequency test\n");
    } else {
        ans.name = "WordsFreq";
        obj->intf->printf("%u-bit words frequency test\n", bits_per_word);
    }
    obj->intf->printf("  nblocks = %lld; block_len = %lld\n",
            (unsigned long long) nblocks,
            (unsigned long long) block_len);
    for (size_t ii = 0; ii < nblocks; ii++) {
        for (size_t i = 0; i < nbins; i++) {
            wfreq[i] = 0;
        }
        for (size_t i = 0; i < block_len; i++) {
            uint64_t u = obj->gi->get_bits(obj->state);        
            for (size_t j = 0; j < nwords_per_num; j++) {
                wfreq[u & mask]++;
                u >>= 8;
            }
        }
        chi2[ii] = 0;
        double Ei = (double) nwords / (double) nbins;
        for (size_t i = 0; i < nbins; i++) {
            chi2[ii] += pow(wfreq[i] - Ei, 2.0) / Ei;
        }
    }
    // Kolmogorov-Smirnov test
    qsort(chi2, nblocks, sizeof(double), cmp_doubles);
    double D = 0.0;
    for (size_t i = 0; i < nblocks; i++) {
        double f = chi2_cdf(chi2[i], nbins - 1);
        double idbl = (double) i;
        double Dplus = (idbl + 1.0) / nblocks - f;
        double Dminus = f - idbl / nblocks;
        if (Dplus > D) D = Dplus;
        if (Dminus > D) D = Dminus;
    }
    double K = sqrt(nblocks) * D + 1.0 / (6.0 * sqrt(nblocks));
    ans.x = K;
    ans.p = ks_pvalue(K);
    ans.alpha = 1.0 - ans.p;
    obj->intf->printf("  K = %g, p = %g\n", K, ans.p);
    obj->intf->printf("\n");
    free(chi2);
    return ans;
}



/**
 * @brief Bytes frequency test.
 */
TestResults byte_freq_test(GeneratorState *obj)
{
    return nbit_words_freq_test(obj, 8, 256);
}

/**
 * @brief 16-bit words frequency test.
 */
TestResults word16_freq_test(GeneratorState *obj)
{
    return nbit_words_freq_test(obj, 16, 16);
}

////////////////////////////////////////////////////
///// Hamming weight (DC6) test implementation /////
////////////////////////////////////////////////////


typedef struct _ByteStreamGenerator {
    const GeneratorInfo *gen; ///< Used generator
    void *state; ///< PRNG state
    uint64_t buffer; ///< Buffer with PRNG raw data
    size_t nbytes; ///< Number of bytes returned by the generator
    size_t bytes_left; ///< Number of bytes left in the buffer
    uint8_t (*get_byte)(struct _ByteStreamGenerator *obj);
} ByteStreamGenerator;


uint8_t ByteStreamGenerator_getbyte(ByteStreamGenerator *obj)
{
    if (obj->bytes_left == 0) {
        obj->buffer = obj->gen->get_bits(obj->state);
        obj->bytes_left = obj->nbytes;
    }
    uint8_t out = obj->buffer & 0xFF;
    obj->buffer >>= 8;
    obj->bytes_left--;
    return out;
}

uint8_t ByteStreamGenerator_getbyte_low1(ByteStreamGenerator *obj)
{
    if (obj->bytes_left == 0) {
        obj->buffer = 0;
        for (int i = 0; i < 64; i++) {
            obj->buffer = (obj->buffer >> 1) | (obj->gen->get_bits(obj->state) << 63);
        }
        obj->bytes_left = 8;
    }
    uint8_t out = obj->buffer & 0xFF;
    obj->buffer >>= 8;
    obj->bytes_left--;
    return out;
}

uint8_t ByteStreamGenerator_getbyte_low8(ByteStreamGenerator *obj)
{
    if (obj->bytes_left == 0) {
        obj->buffer = 0;
        for (int i = 0; i < 8; i++) {
            obj->buffer = (obj->buffer >> 8) | (obj->gen->get_bits(obj->state) << 56);
        }
        obj->bytes_left = 8;
    }
    uint8_t out = obj->buffer & 0xFF;
    obj->buffer >>= 8;
    obj->bytes_left--;
    return out;
}



/**
 * @details It DOESN'T OWN the PRNG state kept in gs->state!
 */
void ByteStreamGenerator_init(ByteStreamGenerator *obj,
    GeneratorState *gs, UseBitsMode use_bits)
{
    obj->gen = gs->gi;
    obj->state = gs->state;
    obj->nbytes = gs->gi->nbits / 8;
    obj->bytes_left = 0;
    switch (use_bits) {
    case use_bits_all:
        obj->get_byte = ByteStreamGenerator_getbyte;
        break;

    case use_bits_low8:
        obj->get_byte = ByteStreamGenerator_getbyte_low8;
        break;

    case use_bits_low1:
        obj->get_byte = ByteStreamGenerator_getbyte_low1;
        break;

    default:
        obj->get_byte = ByteStreamGenerator_getbyte;
        break;
    }
}




/**
 * @brief Keeps the Hamming weight tuple counter and theoretical probability
 */
typedef struct {
    unsigned long long count; ///< Empirical frequency (counter)
    double p; ///< Probability of the corresponding tuple
} HammingWeightsTuple;


typedef struct {
    HammingWeightsTuple *tuples;
    size_t len;
} HammingTuplesTable;


void HammingTuplesTable_init(HammingTuplesTable *obj,
    unsigned int code_nbits, unsigned int tuple_size,
    const double *code_to_prob)
{
    const uint64_t code_mask = (1ull << code_nbits) - 1;
    obj->len = 1ull << code_nbits * tuple_size;
    obj->tuples = calloc(obj->len, sizeof(HammingWeightsTuple));
    if (obj->tuples == NULL) {
        fprintf(stderr, "***** HammingTuplesTable_init: not enough memory *****\n");
        exit(1);
    }
    // Calculate probabilities of tuples
    // (idea is taken from PractRand)
    for (size_t i = 0; i < obj->len; i++) {
        uint64_t tmp = i;
        obj->tuples[i].p = 1.0;
        for (unsigned int j = 0; j < tuple_size; j++) {
            obj->tuples[i].p *= code_to_prob[tmp & code_mask];
            tmp >>= code_nbits;
        }
    }
}

size_t HammingWeightsTuple_reduce_table(HammingWeightsTuple *info, double Ei_min, size_t len)
{
    // Calculate minimal probability for the bin
    unsigned long long count_total = 0;
    for (size_t i = 0; i < len; i++) {
        count_total += info[i].count;
    }
    double p_min = Ei_min / (double) count_total;
    // Bins reduction
    int reduced = 0;
    while (!reduced) {
        size_t out_len = 0, inp_pos = 0;
        reduced = 1;
        while (inp_pos < len) {
            if (info[inp_pos].p >= p_min) {
                // Good bin
                info[out_len++] = info[inp_pos++];
            } else if (inp_pos != len - 1 &&
                (out_len == 0 || info[out_len - 1].p > info[inp_pos + 1].p)) {
                // Bin with low p: attach to the neighbour from the right
                // E.g.: 26 27 | 5* 2 50 => 27 27 7 | ? 50*
                info[out_len] = info[inp_pos++];
                info[out_len].p += info[inp_pos].p;
                info[out_len].count += info[inp_pos].count;
                out_len++; inp_pos++;
                reduced = 0;
            } else {
                // Bin with low p: attach to the neighbour from the left
                // E.g.: 26 27 | 5* 130 50 => 26 32 | ? 130* 50
                info[out_len - 1].p += info[inp_pos].p;
                info[out_len - 1].count += info[inp_pos].count;
                inp_pos++;
                reduced = 0;
            }
        }
        len = out_len;
    }
    return len;
}

/**
 * @brief Calculates statistics for Hamming DC6 test using the filled table
 * of overlapping tuples frequencies. The g-test (refined chi-square test)
 * is used. Obtained random value is converted to the normal distribution
 * using asymptotic approximation from the `chi2_to_stdnorm_approx` function.
 * @details The original DC6 test from PractRand also used recalibration
 * procedure: correction of obtained \f$z_\mathrm{emp}\f$ using some
 * corrections based on percentile tables obtained by Monte-Carlo method using
 * CSPRNG. Experiments showed that increasing \f$E_{i,\mathrm{min}}\f$ from
 * 25 to 100 makes the output distribution much closer to standard normal.
 * It is still slightly biased: \f$ \mu \approx -0.15\f$, $\sigma\approx 1.0$.
 * Q-Q plot shows that it is normal. And this bias was not taken into account.
 */
TestResults HammingTuplesTable_get_results(HammingTuplesTable *obj)
{
    // Concatenate low-populated bins
    const size_t nbins_max = 250000;
    double Ei_min = 250.0;    
    do {
        obj->len = HammingWeightsTuple_reduce_table(obj->tuples, Ei_min, obj->len);
        Ei_min *= 2;
    } while (obj->len > nbins_max);
    // chi2 test (with more accurate g-test statistics)
    // chi2emp = 2\sum_i O_i * ln(O_i / E_i)
    unsigned long long count_total = 0;
    for (size_t i = 0; i < obj->len; i++) {
        count_total += obj->tuples[i].count;
    }
    TestResults ans = TestResults_create("hamming_dc6");
    ans.x = 0;
    for (size_t i = 0; i < obj->len; i++) {
        double Ei = count_total * obj->tuples[i].p;
        double Oi = obj->tuples[i].count;
        if (Oi > DBL_EPSILON) {
            ans.x += 2.0 * Oi * log(Oi / Ei);
        }
    }
    ans.x = chi2_to_stdnorm_approx(ans.x, obj->len - 1);
    ans.p = stdnorm_pvalue(ans.x);
    ans.alpha = stdnorm_cdf(ans.x);    
    return ans;
}

void HammingTuplesTable_free(HammingTuplesTable *obj)
{
    free(obj->tuples);
    obj->tuples = NULL;
}


/**
 * @brief Converts number of samples (bytes or words) to number of generated
 * tuples (not tuples types)
 */
unsigned long long hamming_dc6_nbytes_to_ntuples(unsigned long long nbytes,
    unsigned int nbits, HammingDc6Mode mode)
{
    unsigned long long ntuples = nbytes;
    // Note: tuples are overlapping, i.e. one new tuple digit means
    // one new tuple.
    switch (mode) {
    case hamming_dc6_values:
        // Each PRNG output is converted to tuple digits
        ntuples = nbytes * 8 / nbits;
        break;
    case hamming_dc6_bytes:
        // Each byte is converted to tuple digit
        ntuples = nbytes;
        break;
    case hamming_dc6_bytes_low8:
        // Only lower bytes of PRNG outputs are converted to tuple digits
        ntuples = nbytes * 8 / nbits;
        break;
    case hamming_dc6_bytes_low1:
        // Only lower bits of PRNG outputs are converted to tuple digits
        ntuples = nbytes / nbits;
        break;
    default:
        ntuples = nbytes;
        break;
    }
    return ntuples;
}

/**
 * @brief Print the information about "Hamming DC6" test settings, especially
 * about input stream processing mode.
 */
static void hamming_dc6_test_print_info(GeneratorState *obj, const HammingDc6Options *opts,
    unsigned long long ntuples)
{
    obj->intf->printf("Hamming weights based tests (DC6)\n");
    obj->intf->printf("  Sample size, bytes:     %llu\n", opts->nbytes);
    obj->intf->printf("  Tuples to be generated: %llu\n", ntuples);
    switch (opts->mode) {
    case hamming_dc6_values:
        obj->intf->printf("  Mode: process %d-bit words of PRNG output directly\n",
            (int) obj->gi->nbits);
        break;
    case hamming_dc6_bytes:
        obj->intf->printf("  Mode: process PRNG output as a stream of bytes\n",
            (int) obj->gi->nbits);
        break;
    case hamming_dc6_bytes_low1:
        obj->intf->printf("  Mode: byte stream made of lower bits (bit 0) of PRNG values\n");
        break;
    case hamming_dc6_bytes_low8:
        obj->intf->printf("  Mode: byte stream made of 8 lower bits (bit 7..0) of PRNG values\n");
        break;
    }
}

/**
 * @brief Fills tables for conversion of Hamming weights to its codes (tuple digits)
 * Takes into account both output size of PRNG (32 or 64 bits) and output stream mode
 * (bytes or 32/64-bit words).
 * @param[in]  obj          Information about used PRNG and its state
 * @param[in]  opts         Hamming DC6 test options.
 * @param[out] code_to_prob Buffer for probabilities of codes. They are calculated
 *                          from the binomial distribution. It is supposed that
 *                          only 4 codes (0, 1, 2, 3) are possible
 * @return Pointer to the table of codes of Hamming weights.
 */
static const uint8_t *hamming_dc6_fill_hw_tables(GeneratorState *obj,
    const HammingDc6Options *opts, double *code_to_prob)
{
    // Select the right table for recoding Hamming weights to codes
    int nweights = 0;
    const uint8_t *hw = NULL;
    if (opts->mode != hamming_dc6_values) {
        // 2-bit codes for Hamming weights (taken from PractRand)
        //                                   0  1  2  3  4  5  6  7  8
        static const uint8_t hw_to_code[] = {0, 0, 1, 1, 2, 2, 3, 3, 0};
        hw = hw_to_code;
        nweights = 9;
        // binopdf(0:8,8,0.5) * 256:          1  8 28 56 70 56 28  8  1
    } else if (obj->gi->nbits == 32) {
        // Our custom table of hw->code table for 32-bit words
        // sum(binopdf(0:13, 32,0.5)) ans = 0.1885
        // sum(binopdf(14:15,32,0.5)) ans = 0.2415
        // sum(binopdf(16:17,32,0.5)) ans = 0.2717
        // sum(binopdf(18:32,32,0.5)) ans = 0.2983
        static const uint8_t hw32_to_code[] = {
            0, 0, 0, 0, 0, 0, 0, 0, // 0-7
            0, 0, 0, 0, 0, 0, 1, 1, // 8-15
            2, 2, 3, 3, 3, 3, 3, 3, // 16-23
            3, 3, 3, 3, 3, 3, 3, 3,  3}; // 24-32
        hw = hw32_to_code;
        nweights = 33;
    } else if (obj->gi->nbits == 64) {
        // Our custom table of hw-code table for 64-bit words
        // sum(binopdf(0:28,64,0.5)) ans = 0.1909
        // sum(binopdf(29:31,64,0.5)) ans = 0.2595
        // sum(binopdf(32:35,64,0.5)) ans = 0.3588
        // sum(binopdf(36:64,64,0.5)) ans = 0.1909
        static const uint8_t hw64_to_code[] = {
            0, 0, 0, 0, 0, 0, 0, 0, // 0-7
            0, 0, 0, 0, 0, 0, 0, 0, // 8-15
            0, 0, 0, 0, 0, 0, 0, 0, // 16-23
            0, 0, 0, 0, 0, 1, 1, 1, // 24-31
            2, 2, 2, 2, 3, 3, 3, 3, // 32-39
            3, 3, 3, 3, 3, 3, 3, 3, // 40-47
            2, 2, 3, 3, 3, 3, 3, 3, // 48-57
            3, 3, 3, 3, 3, 3, 3, 3,  3}; // 56-64
        hw = hw64_to_code;
        nweights = 65;
    } else {
        fprintf(stderr, "Internal error");
        exit(1);
    }
    // Calculate probabilities for codes using p.d.f.
    // for binomial distribution
    for (int i = 0; i < 4; i++) {
        code_to_prob[i] = 0.0;
    }
    for (int i = 0; i < nweights; i++) {
        code_to_prob[hw[i]] += binomial_pdf(i, nweights - 1, 0.5);
    }
    return hw;        
}



/**
 * @brief Hamming weights based test (modification of DC6-9x1Bytes-1
 * from PractRand by Chris Doty-Humphrey).
 * @details The test was suggested by Chris Doty-Humphrey, the developer
 * of PractRand test suite. Actually it is a family of algorithms that
 * can analyse bytes, 16-bit, 32-bit and 64-bit chunks. The DC6-9x1Bytes-1
 * modification works with bytes and includes the next steps:
 *
 * 1. Input stream is analysed as a sequence of bytes.
 * 2. Each byte is converted to its Hamming weight (number of 1's).
 * 3. Hamming weight is transformed two-bit binary code using a pre-defined
 *    table. The default settings are tuples from 9 codes.
 *    (number of tuple types: `ntuples = 4^9 = 262144`)
 * 4. A set of overlapping tuples (nsamples, much greater than ntuples) is
 *    formed from the stream, the principle is similar to G.Marsaglia
 *    "monkey tests". Number of tuples of each type is counted, histogram,
 *    i.e. Ei values, are formed. The histogram has `ntuples` bins.
 * 5. The obtained histogram is analysed for bins with small amounts of
 *    elements (<= Ei_min = 25). Such bins are concatenated with neighbours.
 * 6. g-test (slightly more accurate modification of chi-square test) is
 *    appled to the obtained histogram. Then the obtained chi2emp is
 *    transformed to the zemp (standard normal distribution) by some
 *    asymptotic formula.
 * 7. The obtained zemp is recalibrated by some empirical data to improve
 *    the p-values accuracy. The inaccuracy is caused by possible dependencies
 *    between different frequencies (just as in "monkey test"). But the step
 *    with bins concatenation makes theoretical approach problematic. So
 *    Monte-Carlo approach with CSPRNG (HC-256?) was used.
 * 8. p-value is calculated for the obtained zemp.
 *
 * WARNING! This description is reverse engineered from the PractrRand source
 * code by A.L.Voskov and may be inaccurate. Some simplifications were made:
 *
 * 1. Codes reordering was excluded.
 * 2. All generalizations for different shifts/settings were excluded (they
 *    are not used in the default PractRand tests battery).
 * 3. Sophisticated scheme with bits mixing during tuples formation was
 *    excluded (probably the author had some ideas about sorting by
 *    probabilities?)
 * 4. A simpler and less accurate recalibration scheme was developed and used.
 *
 * Also a new mode that processes the whole output of PRNG not byte-by-byte
 * was implemented.
 *
 * The test easily detects low-grade PRNGs such as lcg69069, randu, 32-bit,
 * 64-bit and 128-bit xorshift without output scramblers. If only lower bit is
 * analysed - it detects SWB and SWBW (Subtract-with-Borrow + "Weyl sequence").
 * SWBW passes BigCrush but not PractRand. For such detection the ordering of
 * lower bits in the output is very important.
 * 
 * References:
 *
 * 1. Chris Doty-Humphrey. PractRand https://pracrand.sourceforge.net/
 * 2. Blackman D., Vigna S. A New Test for Hamming-Weight Dependencies //
 *    ACM Trans. Model. Comput. Simul. 2022. V. 32, N. 3, Article 19
 *    https://doi.org/10.1145/3527582
 */
TestResults hamming_dc6_test(GeneratorState *obj, const HammingDc6Options *opts)
{
    // Our custom table of hw->code table for 32-bit words
    double code_to_prob[4];
    const uint8_t *hw_to_code = hamming_dc6_fill_hw_tables(obj, opts, code_to_prob);
    // Parameters for 18-bit tuple with 2-bit digits
    static const unsigned int code_nbits = 2, tuple_size = 9;
    const uint64_t tuple_mask = (1ull << code_nbits * tuple_size) - 1;
    //
    HammingTuplesTable table;
    HammingTuplesTable_init(&table, code_nbits, tuple_size, code_to_prob);

    uint64_t tuple = 0; // 9 4-bit Hamming weights + 1 extra Hamming weight
    uint8_t cur_weight; // Current Hamming weight
    unsigned long long ntuples = hamming_dc6_nbytes_to_ntuples(opts->nbytes,
        obj->gi->nbits, opts->mode);
    hamming_dc6_test_print_info(obj, opts, ntuples);
    obj->intf->printf("  Used probabilities for codes:\n");
    for (int i = 0; i < 4; i++) {
        obj->intf->printf("    p(%d) = %10.8f\n", i, code_to_prob[i]);
    }
    if (opts->mode == hamming_dc6_values) {
        // Process input as a sequence of 32/64-bit words
        // Pre-fill tuple
        cur_weight = get_uint64_hamming_weight(obj->gi->get_bits(obj->state));
        for (unsigned int i = 0; i < tuple_size; i++) {
            tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
            cur_weight = get_uint64_hamming_weight(obj->gi->get_bits(obj->state));
        }
        // Generate other overlapping tuples
        for (unsigned long long i = 0; i < ntuples; i++) {
            // Process current tuple
            table.tuples[tuple].count++;
            // Update tuple
            tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
            cur_weight = get_uint64_hamming_weight(obj->gi->get_bits(obj->state));
        }
    } else {
        ByteStreamGenerator bs;
        if (opts->mode == hamming_dc6_bytes) {
            ByteStreamGenerator_init(&bs, obj, use_bits_all);
        } else if (opts->mode == hamming_dc6_bytes_low8) {
            ByteStreamGenerator_init(&bs, obj, use_bits_low8);
        } else if (opts->mode == hamming_dc6_bytes_low1) {
            ByteStreamGenerator_init(&bs, obj, use_bits_low1);
        } else {
            fprintf(stderr, "Internal error");
            exit(1);
        }
        // Pre-fill tuple
        cur_weight = get_byte_hamming_weight(bs.get_byte(&bs));
        for (unsigned int i = 0; i < tuple_size; i++) {
            tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
            cur_weight = get_byte_hamming_weight(bs.get_byte(&bs));
        }
        // Generate other overlapping tuples
        for (unsigned long long i = 0; i < ntuples; i++) {
            // Process current tuple
            table.tuples[tuple].count++;
            // Update tuple
            tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
            cur_weight = get_byte_hamming_weight(bs.get_byte(&bs));
        }
    }
    // Convert tuples counters to the test results (p-value etc.)
    obj->intf->printf("   Number of tuples types: %llu\n",
        (unsigned long long) table.len);
    TestResults res = HammingTuplesTable_get_results(&table);
    obj->intf->printf("   Number of bins after reduction: %llu\n",
        (unsigned long long) table.len);
    obj->intf->printf("  zemp = %g, p = %g\n", res.x, res.p);
    obj->intf->printf("\n");
    HammingTuplesTable_free(&table);
    return res;
}
